#include "local_proto.h"

void LMS_variogram(struct parameters *var_par, struct write *report)
{
    int nZ, nL;                 // # of gamma matrix rows and columns
    int nr, nc;                 // # of plan matrix rows and columns
    int i, j;                   // indices
    double *h, *vert, *gamma;
    mat_struct *gR;

    nL = var_par->gamma->rows;  // # of cols (horizontal bins)
    nZ = var_par->gamma->cols;  // # of rows (vertical bins)
    gamma = &var_par->gamma->vals[0];   // pointer to experimental variogram

    // Test length of design matrix
    nr = 0;                     // # of rows of design matrix A - depend on # of non-nan gammas
    for (i = 0; i < nZ; i++) {
        for (j = 0; j < nL; j++) {
            if (!isnan(*gamma)) {       // todo: upgrade: !isnan(*c) L434 too
                nr++;
            }
            gamma++;
        }                       // end j
    }                           // end i

    // # of columns of design matrix A
    nc = var_par->function == 5 ? 3 : 1;

    var_par->A = G_matrix_init(nr, nc, nr);     // initialise design matrix
    gR = G_matrix_init(nr, 1, nr);      // initialise vector of observations

    // Set design matrix A
    nr = 0;                     // index of matrix row
    gamma = &var_par->gamma->vals[0];   // pointer to experimental variogram

    if (var_par->function == 5) {       // in case of bivariate variogram
        vert = &var_par->vertical.h[0]; // vertical direction
    }

    for (i = 0; i < nZ; i++) {
        h = var_par->function ==
            5 ? &var_par->horizontal.h[0] : &var_par->h[0];
        for (j = 0; j < nL; j++) {
            if (!isnan(*gamma)) {       // write to matrix - each valid element of gamma
                switch (var_par->function) {    // function of theoretical variogram
                case 0:        // linear variogram
                    G_matrix_set_element(var_par->A, nr, 0, *h);
                    //G_matrix_set_element(var_par->A, nr, 1, 1.);
                    break;
                case 1:        // parabolic variogram
                    G_matrix_set_element(var_par->A, nr, 0, SQUARE(*h));
                    G_matrix_set_element(var_par->A, nr, 1, 1.);
                    break;
                case 5:        // bivariate planar
                    G_matrix_set_element(var_par->A, nr, 0, *h);
                    G_matrix_set_element(var_par->A, nr, 1, *vert);
                    G_matrix_set_element(var_par->A, nr, 2, 1.);
                    break;
                }               // end switch variogram fuction
                G_matrix_set_element(gR, nr, 0, *gamma);
                nr++;           // length of vector of valid elements (not null)          
            }                   // end test if !isnan(*gamma)
            h++;
            gamma++;
        }                       // end j
        if (var_par->function == 5) {
            vert++;
        }
    }                           // end i

    // Estimate theoretical variogram coefficients 
    var_par->T = LSM(var_par->A, gR);   // Least Square Method

    // Test of theoretical variogram estimation 
    if (var_par->T->vals == NULL) {     // NULL values of theoretical variogram
        if (report->write2file == TRUE) {       // close report file
            fprintf(report->fp,
                    "Error (see standard output). Process killed...");
            fclose(report->fp);
        }
        G_fatal_error("Unable to compute 3D theoretical variogram...");
    }                           // error

    // constant raster
    if (var_par->T->vals[0] == 0. && var_par->T->vals[1] == 0.) {
        var_par->const_val = 1;
        G_message(_("Input values to be interpolated are constant."));
    }                           // todo: constant raster for exponential etc.

    // coefficients of theoretical variogram (linear)
    if (report->name) {
        fprintf(report->fp, "Parameters of bivariate variogram:\n");
        fprintf(report->fp, "Function: linear\n\n");
        fprintf(report->fp, "gamma(h, vert) = %f * h + %f * vert + %f\n",
                var_par->T->vals[0], var_par->T->vals[1],
                var_par->T->vals[2]);
    }                           // end if: report
}

double bivar_sill(int direction, mat_struct * gamma)
{
    int i, j, k;
    int n;                      // # of lags
    int n_gamma = 0;            // # of real gammas
    double gamma_i;             // each real gamma
    double sum_gamma = 0.;      // sum of real gammas
    double sill;                // variogram sill

    switch (direction) {        // direction is:
    case 12:                   // horizontal
        n = gamma->rows;        // use 1st column of experimental variogram matrix
        break;
    case 3:                    // vertical
        n = gamma->cols;        // use 1st row of experimental variogram matrix 
        break;
    }

    for (i = 0; i < n; i++) {
        switch (direction) {    // direction is:
        case 12:               // horizontal
            j = i;              // row index is increasing
            k = 0;              // column index is constant
            break;
        case 3:                // vertical
            j = 0;              // row index is constant
            k = i;              // column index is increasing
            break;
        }

        gamma_i = G_matrix_get_element(gamma, j, k);
        if (!isnan(gamma_i)) {  // gamma is real:
            sum_gamma += gamma_i;       // sum all real elements of the matrix
            n_gamma++;          // count them
        }                       // end if
    }                           // end for

    sill = sum_gamma / n_gamma;

    return sill;
}

// Compute sill
void sill(struct parameters *var_par)
{
    // Local variables
    int type = var_par->type;
    mat_struct *gamma = var_par->gamma;

    char var_type[12];

    switch (type) {
    case 2:                    // bivariate variogram
        var_par->horizontal.sill = bivar_sill(12, gamma);
        var_par->vertical.sill = bivar_sill(3, gamma);
        break;

    default:                   // hz / vert / aniso variogram
        var_par->sill = var_par->gamma_sum / var_par->gamma_n;  // mean of gamma elements

        variogram_type(var_par->type, var_type);        // describe code by string
        G_message(_("Default sill of %s variogram: %f"), var_type,
                  var_par->sill);
        break;
    }                           // end switch: variogram type
}

// compare sills
void sill_compare(struct int_par *xD, struct flgs *flg,
                  struct var_par *var_par, struct points *pnts)
{
    // local variables
    double sill_hz = var_par->hz.sill;  // sill in horizontal direction
    double sill_vert = var_par->vert.sill;      // sill in vertical direction

    double diff_sill_05, diff_sill;

    diff_sill_05 = sill_hz > sill_vert ? 0.05 * sill_hz : 0.05 * sill_vert;     // critical value as 5% from bigger sill
    diff_sill = fabsf(sill_hz - sill_vert);     // difference between the sills

    if (xD->bivar == TRUE || (!flg->univariate->answer && diff_sill > diff_sill_05)) {  // zonal anisotropy
        var_par->fin.type = 2;  // code for bivariate variogram
        var_par->fin.td = var_par->hz.td;       // azimuth limit
        var_par->fin.horizontal.max_dist = var_par->hz.max_dist;        // maximum dist in hz direction
        var_par->fin.horizontal.nLag = var_par->hz.nLag;
        var_par->fin.horizontal.lag = var_par->hz.lag;

        var_par->fin.vertical.max_dist = var_par->vert.max_dist;        // maximum dist in vert direction
        var_par->fin.vertical.nLag = var_par->vert.nLag;
        var_par->fin.vertical.lag = var_par->vert.lag;
    }

    else if (xD->univar == TRUE || (!flg->bivariate->answer && diff_sill <= diff_sill_05)) {    // geometric anisotropy 
        var_par->fin.type = 3;  // variogram type: anisotropic
        var_par->fin.max_dist = var_par->hz.max_dist;   // maximum distance: hz
        var_par->fin.td = var_par->hz.td;       // azimuth: hz     
        xD->aniso_ratio = var_par->hz.h_range / var_par->vert.h_range;  // anisotropic ratio
        geometric_anisotropy(xD, pnts); // exaggerate z coords and build a new spatial index 
    }

    return 0;
}

// formulation of variogram functions
double variogram_fction(struct parameters *var_par, double *dr)
{
    // Local variables
    int i;
    int variogram = var_par->function;  // function of theoretical variogram
    int type = var_par->type;   // hz / vert / bivar / aniso
    double nugget;
    double part_sill;
    double h_range;
    double *T;

    if (type == 2 || var_par->function == 0) {  // bivariate variogram: 
        T = &var_par->T->vals[0];       // coefficients of theoretical variogram
    }

    double radius;              // square distance of the point couple
    double h;
    double vert;
    double teor_var, result = 0.;       // estimated value of the theoretical variogram

    int n_cycles = (type == 2 && var_par->function != 5) ? 2 : 1;       // # of cycles (bivariate (not linear) or univariate)

    for (i = 0; i < n_cycles; i++) {
        if (n_cycles == 2) {    // bivariate variogram
            variogram =
                i ==
                0 ? var_par->horizontal.function : var_par->vertical.function;
            h = i == 0 ? dr[0] : dr[1];
            radius = SQUARE(h);
            nugget =
                i ==
                0 ? var_par->horizontal.nugget : var_par->vertical.nugget;
            part_sill =
                i ==
                0 ? var_par->horizontal.sill -
                nugget : var_par->vertical.sill - nugget;
            h_range =
                i ==
                0 ? var_par->horizontal.h_range : var_par->vertical.h_range;
        }
        else {                  // univariate variogram or linear bivariate
            variogram = var_par->function;
            if (variogram == 5) {
                h = dr[0];
                vert = dr[1];
            }
            else {              // univariate variogram
                radius = SQUARE(dr[0]) + SQUARE(dr[1]) + SQUARE(dr[2]); // weighted dz
                h = sqrt(radius);

                nugget = var_par->nugget;
                part_sill = var_par->sill - nugget;
                h_range = var_par->h_range;
            }
        }

        switch (variogram) {
        case 0:                // linear function   
            teor_var = linear(h, *T, *(T + 1));
            break;
        case 1:                // parabolic function TODO: delete
            teor_var = *T * radius + *(T + 1);
            break;
        case 2:                // exponential function
            teor_var = exponential(h, nugget, part_sill, h_range);
            break;
        case 3:                // spherical function
            teor_var = spherical(h, nugget, part_sill, h_range);
            break;
        case 4:                // Gaussian function
            teor_var = gaussian(radius, nugget, part_sill, h_range);
            break;
        case 5:
            teor_var = *T * h + *(T + 1) * vert + *(T + 2);
            break;
        }                       // end switch (variogram)

        result += teor_var;
    }
    if (type == 2 && var_par->function != 5) {
        result -=
            0.5 * (var_par->horizontal.sill - nugget +
                   var_par->vertical.sill - nugget);
    }

    return result;
}

// compute coordinates of reference point - cell centre
void cell_centre(unsigned int col, unsigned int row, unsigned int dep,
                 struct int_par *xD, struct reg_par *reg, double *r0,
                 struct parameters *var_par)
{
    // Local variables
    int i3 = xD->i3;

    r0[0] = reg->west + (col + 0.5) * reg->ew_res;      // x0
    r0[1] = reg->north - (row + 0.5) * reg->ns_res;     // y0

    if (i3 == TRUE) {           // 3D interpolation
        switch (var_par->function) {
        case 5:                // bivariate variogram
            r0[2] = reg->bot + (dep + 0.5) * reg->bt_res;       // z0
            break;
        default:               // univariate (anisotropic) function
            r0[2] = xD->aniso_ratio * (reg->bot + (dep + 0.5) * reg->bt_res);   // z0
            break;
        }                       // end switch
    }                           // end if

    else {                      // 2D interpolation
        r0[2] = 0.;
    }
}

// set up elements of G matrix
mat_struct *set_up_G(struct points *pnts, struct parameters *var_par,
                     struct write *report)
{
    // Local variables
    int n = pnts->n;            // # of input points
    double *r = pnts->r;        // xyz coordinates of input points
    int type = var_par->type;   // hz / vert / aniso / bivar

    int i, j;                   // indices of matrix rows/cols
    int n1 = n + 1;             // # of matrix rows/cols
    double theor_var;           // GM element = theor_var(distance)
    double *dr;                 // dx, dy, dz between point couples
    mat_struct *GM;             // GM matrix

    dr = (double *)G_malloc(3 * sizeof(double));
    GM = G_matrix_init(n1, n1, n1);     // G[n1][n1] matrix

    doublereal *md, *mu, *ml, *dbu, *dbl, *m1r, *m1c;

    dbu = &GM->vals[0];         // upper matrix elements
    dbl = &GM->vals[0];         // lower matrix elements
    md = &GM->vals[0];          // diagonal matrix elements
    m1r = &GM->vals[n1 - 1];    // GM - last matrix row
    m1c = &GM->vals[n1 * n];    // GM - last matrix col

    // G[n;n]
    for (i = 0; i < n1; i++) {  // for elements in each row
        dbu += n1;              // new row of the U
        dbl++;                  // new col of the L
        mu = dbu;               // 1st element in the new U row
        ml = dbl;               // 1st element in the new L col
        for (j = i; j < n1; j++) {      // elements in cols of upper matrix
            if (i != j && i != n && j != n) {   // non-diagonal elements except last row/col
                coord_diff(i, j, r, dr);        // compute coordinate differences
                if (type == 2) {        // bivariate variogram
                    dr[0] = sqrt(radius_hz_diff(dr));
                    dr[1] = dr[2];
                }
                theor_var = variogram_fction(var_par, dr);      // compute GM element

                if (isnan(theor_var)) { // not a number:
                    if (report->name) {
                        fprintf(report->fp,
                                "Error (see standard output). Process killed...");
                        fclose(report->fp);
                    }
                    G_fatal_error(_("Theoretical variogram is NAN..."));
                }

                *mu = *ml = (doublereal) theor_var;     // set the value to the matrix
                mu += n1;       // go to next element in the U row
                ml++;           // go to next element in the L col
            }                   // end non-diagonal elements condition
        }                       // end j loop

        // go to the diagonal element in the next row
        dbu++;                  // U
        dbl += n1;              // L
        *md = 0.0;              // set diagonal
        md += n1 + 1;

        // Set ones
        if (i < n1 - 1) {       // all elements except the last one...
            *m1r = *m1c = 1.0;  // ... shall be 1
            m1r += n1;          // go to next col in last row
            m1c++;              // go to next row in last col
        }                       // end "last 1" condition
    }                           // end i loop

    free(dr);
    return GM;
}

// make G submatrix for rellevant points
mat_struct *submatrix(struct ilist * index, mat_struct * GM_all,
                      struct write * report)
{
    // Local variables
    int n = index->n_values;    // # of selected points
    mat_struct *GM = GM_all;    // whole G matrix

    int i, j, N1 = GM->rows, n1 = n + 1, *dinR, *dini, *dinj;
    doublereal *dbo, *dbx, *dbu, *dbl, *md, *mu, *ml, *m1r, *m1c;

    mat_struct *sub;            // new submatrix

    sub = G_matrix_init(n1, n1, n1);

    if (sub == NULL) {
        if (report->name) {     // close report file
            fprintf(report->fp,
                    "Error (see standard output). Process killed...");
            fclose(report->fp);
        }
        G_fatal_error(_("Unable to initialize G-submatrix..."));
    }

    // Initialize indexes: GM[0;1]
    // - cols
    dini = &index->value[0];    // i   - set processing column

    // initialize new col
    dinR = &index->value[1];    // return to first processing row - lower GM

    dbo = &GM->vals[*dini * N1 + *dinR];        // 1st value in GM_all
    md = &sub->vals[0];         // GM - diagonal
    dbu = &sub->vals[n1];       // GM - upper
    dbl = &sub->vals[1];        // GM - lower
    m1r = &sub->vals[n1 - 1];   // GM - last row
    m1c = &sub->vals[n1 * n];   // GM - last col

    for (i = 0; i <= n; i++) {  // set up cols of submatrix
        // original matrix
        dbx = dbo;              // new col in GM_all
        dinj = dinR;            // orig: inicialize element (row) in (col) - upper matrix
        // submatrix
        mu = dbu;               // sub: start new column
        ml = dbl;               // sub: start new row
        for (j = i + 1; j < n; j++) {   // for rows 
            //submatrix
            *mu = *ml = *dbx;   // general elements
            mu += n1;           // go to element in next column
            ml++;               // go to element in next row
            // Original matrix      
            dinj++;             // set next ind
            dbx += *dinj - *(dinj - 1);
        }                       // end j

        // Original matrix
        dini++;                 // set next ind

        dbu += n1 + 1;          // new col in GM
        dbl += n1 + 1;          // new row in GM

        dinR++;

        dbo += (*dini - *(dini - 1)) * N1 + (*dinR - *(dinR - 1));
        // Set ones     
        *m1r = *m1c = 1.0;
        m1r += n1;
        m1c++;
        // Set diagonals
        *md = 0.0;
        md += n1 + 1;
    }                           // end i

    return sub;
}

// set up g0 vector
mat_struct *set_up_g0(struct int_par * xD, struct points * pnts,
                      struct ilist * index, double *r0,
                      struct parameters * var_par)
{
    // Local variables
    int i3 = xD->i3;            // interpolation: 2D or 3D
    int type = var_par->type;   // variogram: hz / vert / aniso / bivar 
    double *r;                  // xyz coordinates of input points

    int n;                      // # of points (all or selected)
    int n_ind = index->n_values;        // # of selected points
    int *lind;                  // pointer to indices of selected points

    if (n_ind == 0) {           // no selected points:
        n = pnts->n;            // use all points
        r = &pnts->r[0];
    }
    else {                      // any selected points: 
        n = n_ind;              // reduce # of input points
        lind = &index->value[0];
        r = &pnts->r[*lind * 3];
    }

    int n1 = n + 1;

    int i;                      // index of elements and length of g0 vector
    double *dr;                 // coordinate differences dx, dy, dz between couples of points
    mat_struct *g0;             // vector of estimated differences between known and unknown values 

    dr = (double *)G_malloc(3 * sizeof(double));        // Coordinate differences
    g0 = G_matrix_init(n1, 1, n1);

    doublereal *g = g0->vals;

    for (i = 0; i < n; i++) {   // count of input points
        // Coord diffs (input points and cell/voxel center)
        dr[0] = *r - *r0;       // dx
        dr[1] = *(r + 1) - *(r0 + 1);   // dy
        switch (i3) {
        case FALSE:
            // Cell value diffs estimation using linear variogram
            dr[2] = 0.0;        // !!! optimalize - not to be necessary to use this line
            break;
        case TRUE:
            dr[2] = *(r + 2) - *(r0 + 2);       // dz
            break;
        }

        if (type == 2) {        // bivariate variogram
            dr[0] = sqrt(radius_hz_diff(dr));
            dr[1] = dr[2];
        }

        *g = variogram_fction(var_par, dr);     // compute GM element using theoretical variogram function
        g++;

        if (n_ind == 0) {       // no selected points:
            r += 3;             // use each point
        }
        else {                  // selected points:
            lind++;             // go to the next index
            r += 3 * (*lind - *(lind - 1));     // use next selected point
        }
    }                           // end i loop

    // condition: sum of weights = 1
    *g = 1.0;                   // g0 [n1x1]

    return g0;
}

// compute result to be located on reference point
double result(struct points *pnts, struct ilist *index, mat_struct * w0)
{
    // local variables
    int n;                      // # of points
    double *vo;                 // pointer to input values

    int n_ind = index->n_values;        // # of selected points
    int *indo;                  // pointer to indices of selected pt

    // Setup n depending on NN points selection
    n = n_ind == 0 ? pnts->n : n_ind;

    int i;
    mat_struct *ins, *w, *rslt_OK;
    doublereal *vt, *wo, *wt;

    ins = G_matrix_init(n, 1, n);       // matrix of selected vals
    w = G_matrix_init(1, n, 1); // matrix of selected weights

    if (n_ind == 0) {           // there are no selected points:
        vo = &pnts->invals[0];  // use all input values
    }
    else {                      // there are selected points:
        indo = &index->value[0];        // original indexes
        vo = &pnts->invals[*indo];      // original input values
    }

    wo = &w0->vals[0];          // pointer to weights of input vals
    vt = &ins->vals[0];         // pointer to selected inputs values
    wt = &w->vals[0];           // pointer to weights without the last one

    for (i = 0; i < n; i++) {   // for each input point:
        *vt = *vo;              // subval = selected original
        *wt = *wo;              // sub weight = selected original
        //G_debug(0,"%f %f", *vt, *wt);

        // go to the next:
        if (n_ind == 0) {       // use all points:
            vo++;               // input value
        }
        else {                  // use selected points:
            indo++;             // index of selected point
            vo += *indo - *(indo - 1);  // selected input value
        }
        vt++;                   // element of value matrix
        wo++;                   // weight of the value 
        wt++;                   // element of weight matrix
    }                           // end i for loop

    rslt_OK = G_matrix_product(w, ins); // interpolated value

    G_matrix_free(w);
    G_matrix_free(ins);

    return rslt_OK->vals[0];
}

// validation
void crossvalidation(struct int_par *xD, struct points *pnts,
                     struct parameters *var_par)
{
    int n = pnts->n;            // # of input points
    double *r = pnts->r;        // coordinates of points
    int i3 = xD->i3;
    double *vals = pnts->invals;        // input values

    int type = var_par->type;
    double ratio = type == 3 ? xD->aniso_ratio : 1.;    // anisotropic ratio
    mat_struct *GM = var_par->GM;       // GM = theor_var(distances)

    int i;
    int n_vals;
    double max_dist =
        type == 2 ? var_par->horizontal.max_dist : var_par->max_dist;
    double max_dist_vert =
        type == 2 ? var_par->vertical.max_dist : var_par->max_dist;

    double *search;             // coordinates of the search point
    struct ilist *list;
    mat_struct *GM_sub;
    mat_struct *GM_Inv, *g0, *w0;
    double rslt_OK;

    struct write *crossvalid = &xD->crossvalid;
    struct write *report = &xD->report;
    double *normal, *absval, *norm, *av;

    search = (double *)G_malloc(3 * sizeof(double));
    normal = (double *)G_malloc(n * sizeof(double));
    absval = (double *)G_malloc(n * sizeof(double));
    norm = &normal[0];
    av = &absval[0];

    FILE *fp;

    if (crossvalid->name) {     // CV report file available:
        fp = fopen(crossvalid->name, "w");
    }

    for (i = 0; i < n; i++) {   // for each input point [r0]:
        list = G_new_ilist();   // create list of overlapping rectangles

        search = &pnts->r[3 * i];
        if (i3 == TRUE) {
            list = find_NNs_within(3, search, pnts, max_dist, max_dist_vert);
        }
        else {
            list = find_NNs_within(2, search, pnts, max_dist, max_dist_vert);
        }

        n_vals = list->n_values;        // # of overlapping rectangles

        if (n_vals > 0) {       // if positive:
            correct_indices(list, r, pnts, var_par);

            GM_sub = submatrix(list, GM, &xD->report);  // create submatrix using indices
            GM_Inv = G_matrix_inverse(GM_sub);  // inverse matrix
            G_matrix_free(GM_sub);

            g0 = set_up_g0(xD, pnts, list, r, var_par); // Diffs inputs - unknowns (incl. cond. 1)) 
            w0 = G_matrix_product(GM_Inv, g0);  // Vector of weights, condition SUM(w) = 1 in last row

            G_matrix_free(g0);
            G_matrix_free(GM_Inv);

            rslt_OK = result(pnts, list, w0);   // Estimated cell/voxel value rslt_OK = w x inputs
            G_matrix_free(w0);

            //Create output 
            *norm = rslt_OK - *vals;    // differences between input and interpolated values
            *av = fabsf(*norm); // absolute values of the differences (quantile computation)

            if (xD->i3 == TRUE) {       // 3D interpolation:
                fprintf(fp, "%d %.3f %.3f %.2f %f %f %f\n", i, *r, *(r + 1),
                        *(r + 2) / ratio, pnts->invals[i], rslt_OK, *norm);
            }
            else {              // 2D interpolation:
                fprintf(fp, "%d %.3f %.3f %f %f %f\n", i, *r, *(r + 1), *vals,
                        rslt_OK, *norm);
            }
        }                       // end if: n_vals > 0

        else {                  // n_vals == 0:
            fprintf(fp,
                    "%d. point does not have neighbours in given radius\n",
                    i);
        }

        r += 3;                 // next rectangle
        vals++;
        norm++;
        av++;

        G_free_ilist(list);     // free list memory
    }                           // end i for loop

    fclose(fp);
    G_message(_("Cross validation results have been written into <%s>"),
              crossvalid->name);

    if (report->name) {
        double quant95;

        fprintf(report->fp,
                "\n************************************************\n");
        fprintf(report->fp, "*** Cross validation results ***\n");

        test_normality(n, normal, report);

        fprintf(report->fp, "Quantile of absolute values\n");
        quant95 = quantile(0.95, n, absval, report);
    }
}
