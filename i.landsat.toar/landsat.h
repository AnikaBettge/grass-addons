#ifndef _LANDSAT_H
#define _LANDSAT_H

#define MAX_BANDS   9

#define UNCORRECTED     0
#define CORRECTED       1
#define SIMPLIFIED      2


/*****************************************************
 * Landsat Structures
 *
 * Lmax and Lmin in  W / (m� � sr � �m) -> Radiance
 * Esun in  W / (m� � �m)               -> Irradiance
 ****************************************************/

typedef struct
{
    int number;			/* Band number                   */
    int code;                   /* Band code                     */

    double wavemax, wavemin;    /* Wavelength in �m              */

    double lmax, lmin;		/* Spectral radiance             */
    double qcalmax, qcalmin;	/* Quantized calibrated pixel    */
    double esun;                /* Mean solar irradiance         */

    char thermal;               /* Flag to thermal band          */
    double gain, bias;          /* Gain and Bias of sensor       */
    double K1, K2;              /* Thermal calibration constants,
                                   or Rad2Ref constants          */

} band_data;

typedef struct
{
    unsigned char number;       /* Landsat number                */

    char creation[11];          /* Image production date         */
    char date[11];              /* Image acquisition date        */
    double dist_es;		/* Distance Earth-Sun            */
    double sun_elev;		/* Solar elevation               */

    char sensor[5];             /* Type of sensor: MSS, TM, ETM+ */
    int bands;			/* Total number of bands         */
    band_data band[MAX_BANDS];	/* Data for each band            */

} lsat_data;


/*****************************************************************************
 * Landsat Equations Prototypes
 *****************************************************************************/

double lsat_qcal2rad(int, band_data *);
double lsat_rad2ref(double, band_data *);
double lsat_rad2temp(double, band_data *);

void lsat_bandctes(lsat_data *, int, char, double, double);

#endif
