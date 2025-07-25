#include <stdio.h>
#include <math.h>

typedef struct {
    int Ntrx;          // Total number of TRX chains (antennas)
    double Prf;        // RF transceiver power per chain (W)
    double Pbb;        // Baseband power per chain (W)
    double sigma_dc;   // DC-DC loss factor (0-1)
    double sigma_ms;   // Main supply loss factor (0-1)
    double sigma_feed; // Feeder loss factor (0-1)
    double sigma_cool; // Cooling loss factor (0-1)
    double eta_pa;     // PA efficiency (0-1)
    double Pout_max;   // Max output power per antenna (W)
    double Psleep;     // Sleep mode power (W)
} BS_PowerModel;

// Compute component breakdown using full EARTH model formula
void compute_components(double Pout, BS_PowerModel bs, 
                        double *PA, double *RF, double *BB, 
                        double *DC, double *MS, double *CO) {
    if (Pout == 0.0) {
        *PA = *RF = *BB = *DC = *MS = *CO = 0.0;
        return;
    }

    // PA input power (before feeder loss and PA efficiency)
    double pa_input = Pout / (bs.eta_pa * (1.0 - bs.sigma_feed));
    *PA = bs.Ntrx * pa_input;

    // RF and BB power
    *RF = bs.Ntrx * bs.Prf;
    *BB = bs.Ntrx * bs.Pbb;

    // Sum of PA, RF, BB
    double num = *PA + *RF + *BB;

    // Apply power supply chain losses
    double denom = (1.0 - bs.sigma_dc) * (1.0 - bs.sigma_ms) * (1.0 - bs.sigma_cool);
    double Pin = num / denom;

    // Reconstruct losses
    double after_cool = Pin * (1.0 - bs.sigma_cool);
    double after_ms = after_cool * (1.0 - bs.sigma_ms);
    double after_dc = after_ms * (1.0 - bs.sigma_dc);

    *DC = after_ms - after_dc;
    *MS = after_cool - after_ms;
    *CO = Pin - after_cool;
}

int main() {
    // EARTH macro BS parameters
    BS_PowerModel macro = {
        .Ntrx = 6,
        .Prf = 13,
        .Pbb = 29.5,
        .sigma_dc = 0.075,
        .sigma_ms = 0.09,
        .sigma_feed = 0.05,
        .sigma_cool = 0.1,
        .eta_pa = 0.311,
        .Pout_max = 128.2,
        .Psleep = 75.0
    };

    const int N = 21;
    FILE *data = fopen("components.dat", "w");
    if (!data) return 1;

    fprintf(data, "Load_Percent Sleep PA RF BB DC MS CO Total\n");

    for (int i = 0; i < N; i++) {
        double load_percent = i * 5.0;
        double load_factor = load_percent / 100.0;
        double Pout = (load_percent == 0.0) ? 0.0 : load_factor * macro.Pout_max;

        double PA, RF, BB, DC, MS, CO;
        compute_components(Pout, macro, &PA, &RF, &BB, &DC, &MS, &CO);

        double sleep_power = (Pout == 0.0) ? macro.Psleep : 0.0;
        double total_power = sleep_power + PA + RF + BB + DC + MS + CO;

        fprintf(data, "%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
                load_percent, sleep_power, PA, RF, BB, DC, MS, CO, total_power);
    }

    fclose(data);

    // Launch gnuplot for visualization
    FILE *gp = popen("gnuplot -persistent", "w");
    if (!gp) return 1;

    fprintf(gp, "set title 'Macrocell BS Power'\n");
    fprintf(gp, "set xlabel 'RF Output Power (%% of Max)'\n");
    fprintf(gp, "set ylabel 'Power Consumption (W)'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set key outside right top vertical\n");
    fprintf(gp, "set xrange [0:100]\n");
    fprintf(gp, "set yrange [0:1500]\n");
    fprintf(gp, "set style fill transparent solid 0.7\n");

    fprintf(gp,
        "set style line 1 lc rgb '#888888'\n"
        "set style line 2 lc rgb '#FF0000'\n"
        "set style line 3 lc rgb '#0000FF'\n"
        "set style line 4 lc rgb '#00AA00'\n"
        "set style line 5 lc rgb '#AA00FF'\n"
        "set style line 6 lc rgb '#FF8800'\n"
        "set style line 7 lc rgb '#AA5500'\n"
        "set style line 8 lc rgb '#000000'\n");

    fprintf(gp,
        "plot 'components.dat' using 1:($2==0 ? 0 : 1/0):(0) with filledcurves y1 title 'Sleep Mode' ls 1, \\\n"
        "     '' using 1:($2):($2+$3) with filledcurves title 'PA (Power Amplifier)' ls 2, \\\n"
        "     '' using 1:($2+$3):($2+$3+$4) with filledcurves title 'RF (RF Transceiver)' ls 3, \\\n"
        "     '' using 1:($2+$3+$4):($2+$3+$4+$5) with filledcurves title 'BB (Baseband)' ls 4, \\\n"
        "     '' using 1:($2+$3+$4+$5):($2+$3+$4+$5+$6) with filledcurves title 'DC (DC-DC Converters)' ls 5, \\\n"
        "     '' using 1:($2+$3+$4+$5+$6):($2+$3+$4+$5+$6+$7) with filledcurves title 'PS (AC/DC Power Supply)' ls 6, \\\n"
        "     '' using 1:($2+$3+$4+$5+$6+$7):($2+$3+$4+$5+$6+$7+$8) with filledcurves title 'CO (Cooling)' ls 7, \\\n"
        "     '' using 1:9 with lines ls 8 lw 2 title 'Total Power'\n");

    pclose(gp);
    return 0;
}

