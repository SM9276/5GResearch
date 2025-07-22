#include <stdio.h>
#include <math.h>

typedef struct {
    int Ntrx;          // Total number of TRX chains (antennas)
    double Prf;        // RF transceiver power per chain (W)
    double Pbb;        // Baseband power per chain (W)
    double sigma_dc;   // DC-DC loss factor (0-1)
    double sigma_ms;   // Main supply loss factor (0-1)
    double eta_pa;     // PA efficiency (0-1)
    double Pout_max;   // Max output power per antenna (W)
    double Psleep;     // Sleep mode power (W)
} BS_PowerModel;

// Compute component breakdown with correct relationships
void compute_components(double Pout, BS_PowerModel bs, 
                        double *PA, double *RF, double *BB, 
                        double *DC, double *MS, double *CO) {
    // Zero out components if in sleep mode
    if (Pout == 0.0) {
        *PA = *RF = *BB = *DC = *MS = *CO = 0.0;
        return;
    }
    
    // PA component (DC input power to PA)
    *PA = bs.Ntrx * (Pout / bs.eta_pa);
    
    // RF transceiver power (fixed)
    *RF = bs.Ntrx * bs.Prf;
    
    // Baseband power (fixed)
    *BB = bs.Ntrx * bs.Pbb;
    
    // Total active power (PA + RF + BB)
    double total_active = *PA + *RF + *BB;
    
    // DC-DC converter loss (proportional to active power)
    *DC = total_active * bs.sigma_dc;
    
    // Power after DC-DC conversion
    double after_dc = total_active + *DC;
    
    // Main supply loss (proportional to DC-DC output)
    *MS = after_dc * bs.sigma_ms;
    
    // Cooling is zero for RRH configuration
    *CO = 0.0;
}

int main() {
    // Macro BS parameters for RRH configuration (matches EARTH model)
    BS_PowerModel macro = {
        .Ntrx = 6,         // 6 TRX chains (2 antennas/sector × 3 sectors)
        .Prf = 8.0,        // RF transceiver power per chain
        .Pbb = 8.0,        // Baseband power per chain
        .sigma_dc = 0.075, // 7.5% DC-DC loss
        .sigma_ms = 0.09,  // 9% main supply loss
        .eta_pa = 0.38,    // PA efficiency
        .Pout_max = 20.0,  // Max output power per antenna (W)
        .Psleep = 75.0     // Sleep mode power
    };

    const int N = 21;  // 0% to 100% in 5% steps
    FILE *data = fopen("components.dat", "w");
    if (!data) return 1;
    
    // Write header for stacked area plot
    fprintf(data, "Load_Percent Sleep PA RF BB DC MS CO Total\n");
    
    for (int i = 0; i < N; i++) {
        double load_percent = i * 5.0;  // 0%, 5%, ..., 100%
        double load_factor = load_percent / 100.0;
        double Pout = (load_percent == 0.0) ? 0.0 : load_factor * macro.Pout_max;
        
        // Compute components
        double PA, RF, BB, DC, MS, CO;
        compute_components(Pout, macro, &PA, &RF, &BB, &DC, &MS, &CO);
        
        double sleep_power = (load_percent == 0.0) ? macro.Psleep : 0.0;
        double total_power = sleep_power + PA + RF + BB + DC + MS + CO;
        
        // Write components in stacking order
        fprintf(data, "%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n", 
                load_percent, sleep_power, PA, RF, BB, DC, MS, CO, total_power);
    }
    fclose(data);

    // Generate properly stacked area plot
    FILE *gp = popen("gnuplot -persistent", "w");
    if (!gp) return 1;

    fprintf(gp, "set title 'Macrocell BS Power Component Breakdown (RRH Configuration)'\n");
    fprintf(gp, "set xlabel 'RF Output Power (%% of Max)'\n");
    fprintf(gp, "set ylabel 'Power Consumption (W)'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set key outside right top vertical\n");
    fprintf(gp, "set xrange [0:100]\n");
    fprintf(gp, "set yrange [0:700]\n");
    fprintf(gp, "set style fill transparent solid 0.7\n\n");
    
    fprintf(gp, "# Define custom colors\n");
    fprintf(gp, "set style line 1 lc rgb '#888888'  # Sleep - Gray\n");
    fprintf(gp, "set style line 2 lc rgb '#FF0000'  # PA - Red\n");
    fprintf(gp, "set style line 3 lc rgb '#0000FF'  # RF - Blue\n");
    fprintf(gp, "set style line 4 lc rgb '#00AA00'  # BB - Green\n");
    fprintf(gp, "set style line 5 lc rgb '#AA00FF'  # DC - Purple\n");
    fprintf(gp, "set style line 6 lc rgb '#FF8800'  # PS (AC/DC) - Orange\n");
    fprintf(gp, "set style line 7 lc rgb '#AA5500'  # CO - Brown\n");
    fprintf(gp, "set style line 8 lc rgb '#000000'  # Total - Black\n\n");
    
    fprintf(gp, "# Plot stacked components with correct boundaries\n");
    fprintf(gp, "plot 'components.dat' using 1:2 with filledcurves title 'Sleep Mode' ls 1, \\\n");
    fprintf(gp, "     '' using 1:($2):($2+$3) with filledcurves title 'PA (Power Amplifier)' ls 2, \\\n");
    fprintf(gp, "     '' using 1:($2+$3):($2+$3+$4) with filledcurves title 'RF (RF Transceiver)' ls 3, \\\n");
    fprintf(gp, "     '' using 1:($2+$3+$4):($2+$3+$4+$5) with filledcurves title 'BB (Baseband)' ls 4, \\\n");
    fprintf(gp, "     '' using 1:($2+$3+$4+$5):($2+$3+$4+$5+$6) with filledcurves title 'DC (DC-DC Converters)' ls 5, \\\n");
    fprintf(gp, "     '' using 1:($2+$3+$4+$5+$6):($2+$3+$4+$5+$6+$7) with filledcurves title 'PS (AC/DC Power Supply)' ls 6, \\\n");
    fprintf(gp, "     '' using 1:($2+$3+$4+$5+$6+$7):($2+$3+$4+$5+$6+$7+$8) with filledcurves title 'CO (Cooling)' ls 7, \\\n");
    fprintf(gp, "     '' using 1:9 with lines ls 8 lw 2 title 'Total Power'\n");
    
    pclose(gp);
    return 0;
}
