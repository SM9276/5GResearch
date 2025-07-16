#include <stdio.h>

typedef struct {
    int Ntrx;           // Number of transceivers
    int Npa;            // Number of power amplifiers
    double Prf;         // RF power (W)
    double Pbb;         // Baseband power (W)
    double sigma_feed;  // Feeder loss
    double sigma_dc;    // DC-DC loss
    double sigma_ms;    // Main supply loss
    double sigma_cool;  // Cooling loss
    double Pout_max;    // Max RF output (W)
} BS_PowerModel;

// EARTH BS power consumption
double compute_Pin(double Pout, BS_PowerModel bs) {
    double numerator = (Pout / (bs.Npa * (1.0 - bs.sigma_feed))) + bs.Prf + bs.Pbb;
    double efficiency = (1.0 - bs.sigma_dc) * (1.0 - bs.sigma_ms) * (1.0 - bs.sigma_cool);
    return bs.Ntrx * (numerator / efficiency);
}

int main() {
    BS_PowerModel macro = {
        .Ntrx = 3,
        .Npa = 2,
        .Prf = 13.0,
        .Pbb = 29.5,
        .sigma_feed = 0.1,
        .sigma_dc = 0.075,
        .sigma_ms = 0.09,
        .sigma_cool = 0.1,
        .Pout_max = 39.5
    };

    FILE *data = fopen("bs_power_watts.dat", "w");
    if (!data) return 1;

    for (double x = 0.0; x <= 1.001; x += 0.05) {
        double Pout = x * macro.Pout_max;
        double Pin = compute_Pin(Pout, macro);
        fprintf(data, "%f %f\n", x * 100.0, Pin); // X: RF % | Y: Power in Watts
    }
    fclose(data);

    FILE *gp = popen("gnuplot -persistent", "w");
    if (!gp) return 1;

    fprintf(gp, "set title 'MACRO'\n");
    fprintf(gp, "set xlabel 'RF Output Power (%% of Max)'\n");
    fprintf(gp, "set ylabel 'BS Power Consumption (W)'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "plot 'bs_power_watts.dat' using 1:2 with linespoints title 'Macrocell BS'\n");

    pclose(gp);
    return 0;
}

