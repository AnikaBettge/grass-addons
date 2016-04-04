#!/usr/bin/env python
############################################################################
#
# MODULE:       r.randomforest
# AUTHOR:       Steven Pawley
# PURPOSE:      Provides supervised random forest classification and regression
#               (using python scikit-learn and pandas)
#
# COPYRIGHT:    (c) 2015 Steven Pawley, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Provides supervised random forest classification
#% keyword: classification
#% keyword: machine learning
#% keyword: scikit-learn
#% keyword: pandas
#% keyword: random forests
#%end

#%option G_OPT_I_GROUP
#% key: igroup
#% label: Imagery group to be classified (predictors)
#% description: Series of raster maps to be used in the random forest classification
#% required: yes
#% multiple: no
#%end

#%option G_OPT_R_INPUT
#% key: roi
#% label: Raster map with labelled pixels
#% description: Raster map with labelled pixels
#% required: yes
#%end

#%option G_OPT_R_OUTPUT
#% key: output
#% required: yes
#% label: Output Map
#%end

#%option string
#% key: mode
#% required: yes
#% label: Classification or regression mode
#% answer: classification
#% options: classification,regression
#%end

#%option
#% key: ntrees
#% type: integer
#% description: Number of trees in the forest
#% answer: 500
#% required: yes
#% guisection: Random Forest Options
#%end

#%option
#% key: mfeatures
#% type: integer
#% description: The number of features allowed at each split. Sqrt(n_features) is used by default
#% answer: -1
#% required: yes
#% guisection: Random Forest Options
#%end

#%option
#% key: minsplit
#% type: integer
#% description: The minimum number of samples required to split an internal node
#% answer: 2
#% required: yes
#% guisection: Random Forest Options
#%end

#%option
#% key: randst
#% type: integer
#% description: Seed to pass onto the random state for reproducible results
#% answer: 1
#% required: yes
#% guisection: Random Forest Options
#%end

#%option
#% key: lines
#% type: integer
#% description: Processing block size in terms of number of rows
#% answer: 20
#% required: yes
#% guisection: Optional
#%end

#%flag
#% key: p
#% label: Output class membership probabilities
#% guisection: Random Forest Options
#%end

#%flag
#% key: b
#% description: Balance classes by weighting
#% guisection: Random Forest Options
#%end

#%flag
#% key: m
#% description: Build model only - do not perform prediction
#% guisection: Random Forest Options
#%end

#%option G_OPT_F_OUTPUT
#% key: savefile
#% label: Save model from file
#% required: no
#% guisection: Optional
#%end

#%option G_OPT_F_INPUT
#% key: loadfile
#% label: Load model from file
#% required: no
#% guisection: Optional
#%end

#%option G_OPT_F_OUTPUT
#% key: fimportance
#% label: Save feature importance and accuracy to csv
#% required: no
#% guisection: Optional
#%end

# standard modules
from __future__ import print_function
import atexit, os, random, string, imp
from grass.pygrass.raster import RasterRow
from grass.pygrass.gis.region import Region
from grass.pygrass.raster.buffer import Buffer
import grass.script as grass
import numpy as np

# non-standard modules
def module_exists(module_name):
    try:
        imp.find_module(module_name)
        return True
    except ImportError:
        grass.error(_("{} Python package not installed."
                      " Exiting").format(module_name))
        return False

if module_exists("sklearn") == True:
    from sklearn.ensemble import RandomForestClassifier, RandomForestRegressor
    from sklearn.externals import joblib
else:
    exit()
if module_exists("pandas") == True:
    import pandas as pd
else:
    exit()

def cleanup():
    # We can then close the rasters and the roi image
    for i in range(nbands): rasstack[i].close()
    mask_raster.close()
    roi_raster.close()
    grass.run_command("g.remove", name=rfmask, flags="f", type="raster")

def normalize_newlines(string):
    # Windows uses carriage return and line feed ("\r\n") as a line ending
    # Unix uses just line feed ("\n"). This function normalizes strings to "\n"
    import re
    return re.sub(r'(\r\n|\r|\n)', '\n', string)

def main():
    igroup = options['igroup']
    roi = options['roi']
    output = options['output']
    mode = options['mode']
    ntrees = options['ntrees']
    balanced = flags['b']
    modelonly = flags['m']
    class_probabilities = flags['p']
    rowincr = int(options['lines'])
    mfeatures = int(options['mfeatures'])
    minsplit = int(options['minsplit'])
    randst = int(options['randst'])
    model_save = options['savefile']
    model_load = options['loadfile']
    fimportance = options['fimportance']

    ##################### error checking for valid input parameters ################################
    if mfeatures == -1:
        mfeatures = str('auto')
    if mfeatures == 0:
        print("mfeatures must be greater than zero, or -1 which uses the sqrt(nfeatures)...exiting")
        exit()
    if minsplit == 0:
        print("minsplit must be greater than zero.....exiting")
        exit()
    if rowincr <= 0:
        print("rowincr must be greater than zero....exiting")
        exit()
    if ntrees < 1:
        print("ntrees must be greater than zero.....exiting")
        exit()
    if mode == 'regression' and balanced == True:
        print ("balanced mode is ignored in Random Forests in regression mode....continuing")
    if mode == 'regression' and class_probabilities == True:
        print ("option to output class probabiltities is ignored in regression mode....continuing")
    if model_save != '' and model_load != '':
        print("Cannot save and load a model at the same time")
        exit()

    ######################  Fetch individual raster names from group ###############################
    groupmaps = grass.read_command("i.group", group=igroup, flags="g")
    groupmaps = normalize_newlines(groupmaps)
    maplist = groupmaps.split('\n')
    maplist = maplist[0:len(maplist)-1]

    ######################### Obtain information about GRASS rasters to be classified ##############

    # Determine number of bands and then create a list of GRASS rasterrow objects
    global nbands
    nbands = len(maplist)

    global rasstack
    rasstack = [0] * nbands
    for i in range(nbands):
        rasstack[i] = RasterRow(maplist[i])

    # Check to see if each raster in the list exists
    for i in range(nbands):
        if rasstack[i].exist() == True:
            rasstack[i].open('r')
        else:
            print("GRASS raster " + maplist[i] + " does not exist.... exiting")
            exit()

    # Use grass.pygrass.gis.region to get information about the current region, particularly
    # the number of rows and columns. We are going to sample and classify the data row-by-row,
    # and not load all of the rasters into memory in a numpy array
    current = Region()

    ########################### Create a imagery mask ##############################################
    # The input rasters might have different dimensions in terms of value and non-value pixels.
    # We will use the r.series module to automatically create a mask by propagating the null values

    global rfmask

    rfmask = 'tmp_' + ''.join([random.choice(string.ascii_letters + string.digits) \
    for n in xrange(8)])

    grass.run_command("r.series", output=rfmask, input=maplist, method='count', flags='n', \
    overwrite=True)

    global mask_raster
    mask_raster = RasterRow(rfmask)
    mask_raster.open('r')

    ######################### Sample training data using training ROI ##############################
    global roi_raster
    roi_raster = RasterRow(roi)

    if roi_raster.exist() == True:
        roi_raster.open('r')
    else:
        print("ROI raster does not exist.... exiting")
        exit()

    # load the model
    if model_load != '':
        rf = joblib.load(model_load)
    else:
        # determine cell storage type of training roi raster
        roi_type = grass.read_command("r.info", map=roi, flags='g')
        roi_type = normalize_newlines(str(roi_type))
        roi_list = roi_type.split('\n')
        dtype = roi_list[9].split('=')[1]

        # check if training rois are valid for classification and regression
        if mode == 'classification' and dtype != 'CELL':
            print ("Classification mode requires an integer CELL type training roi map.....exiting")
            exit()

        # Count number of labelled pixels
        roi_stats = str(grass.read_command("r.univar", flags=("g"), map=roi))
        if os.name == "nt":
            roi_stats = roi_stats[0:len(roi_stats)-2]
            roi_stats = roi_stats.split('\r\n')[0]
        else:
            roi_stats = roi_stats[0:len(roi_stats)-1]
            roi_stats = roi_stats.split('\n')[0]

        ncells = str(roi_stats).split('=')[1]
        nlabel_pixels = int(ncells)

        # Create a zero numpy array with the dimensions of the number of columns in the region
        # and the number of bands plus an additional band to attach the labels
        tindex = 0
        training_labels = []
        training_data = np.zeros((nlabel_pixels, nbands+1))
        training_data[:] = np.NAN

        # Loop through each row of the raster, get the pixels from that row in the ROI raster
        # and check if any of those pixels are labelled (i.e. they are not nan).
        # If labelled pixels are encountered, loop through each band for that row and put the data
        # into the img_row_band_np array. Also attach the label values onto the last column

        for row in range(current.rows):
            roi_row_np = roi_raster[row]
            is_train = np.nonzero(roi_row_np > -2147483648)
            training_labels = np.append(training_labels, roi_row_np[is_train])
            nlabels_in_row = np.array(is_train).shape[1]
            if np.isnan(roi_row_np).all() != True:
                for band in range(nbands):
                    imagerow_np = rasstack[band][row]
                    training_data[tindex : tindex+nlabels_in_row, band] = imagerow_np[is_train]
                tindex = tindex + nlabels_in_row

        # Determine the number of class labels using np.unique
        nclasses = len(np.unique(training_labels))
        class_list = np.unique(training_labels)

        # attach training label values onto last dimension of numpy array
        training_data[0:nlabel_pixels, nbands] = training_labels

        # Remove nan rows from numpy array
        training_data = training_data[~np.isnan(training_data).any(axis=1)]

        # Split the numpy array into training_labels and training_data arrays
        training_labels = training_data[:, nbands]
        training_data = training_data[:, 0:nbands]

    ############################### Training the classifier #######################################
    if model_load == '':
        if mode == 'classification':
            if balanced == True:
                rf = RandomForestClassifier(n_jobs=-1, n_estimators=int(ntrees), oob_score=True, \
                class_weight='balanced', max_features=mfeatures, min_samples_split=minsplit, \
                random_state=randst)
            else:
                rf = RandomForestClassifier(n_jobs=-1, n_estimators=int(ntrees), oob_score=True, \
                max_features=mfeatures, min_samples_split=minsplit, random_state=randst)
            rf = rf.fit(training_data, training_labels)
            acc = float(rf.oob_score_)
            print('Our OOB prediction of accuracy is: {oob}%'.format(oob=rf.oob_score_ * 100))
        else:
            rf = RandomForestRegressor(n_jobs=-1, n_estimators=int(ntrees), oob_score=True, \
            max_features=mfeatures, min_samples_split=minsplit, random_state=randst)
            rf = rf.fit(training_data, training_labels)
            acc = rf.score(X=training_data, y=training_labels)
            print('Our coefficient of determination R^2 of the prediction is: {r2}%'.format \
            (r2=rf.score(X=training_data, y=training_labels)))

        # diagnostics
        rfimp = pd.DataFrame(rf.feature_importances_)
        rfimp.insert(loc=0, column='Raster', value=maplist)
        rfimp.columns = ['Raster', 'Importance']
        print(rfimp)
        
        # save diagnostics to file        
        if fimportance != '':
            rfimp.to_csv(path_or_buf = fimportance)
            fd = open(fimportance, 'a')
            fd.write(str(rfimp.shape[0]) + ',' + 'Performance measure (OOB or R2)' + ',' + str(acc))
            fd.close()
        
        # save the model
        if model_save != '':
            joblib.dump(rf, model_save + ".pkl")
        
        if modelonly == True:
            exit()

    ################################ Prediction on the rest of the raster stack ###################
    # 1. Create a np.array that can store each raster row for all of the bands
    # 2. Loop through the raster, row-by-row and get the row values for each band
    #    adding these to the img_np_row np.array,
    #    which bundles rowincr rows together to pass to the classifier,
    #    otherwise, row-by-row is too inefficient.
    # 3. The scikit learn predict function expects a list of pixels, not an NxM matrix.
    #    We therefore need to reshape each row matrix into a list.
    #    The total matrix size = cols * nbands.
    #    Therefore we can use the np.reshape function to convert the image into a list
    #    with the number of rows = n_samples, and the number of columns = the number of bands.
    # 4. Then we remove any NaN values because the scikit-learn predict function cannot handle NaNs.
    #    Here we replace them with a small value using the np.nan_to_num function.
    # 5. The flat_pixels is then passed onto the prediction function.
    # 6. After the prediction is performed on the row, to save keeping
    #    anything in memory, we save it to a GRASS raster object, row-by-row.

    classification = RasterRow(output)
    if mode == 'classification':
        ftype = 'CELL'
        nodata = -2147483648
    else:
        ftype = 'FCELL'
        nodata = np.nan
    classification.open('w', ftype, overwrite=True)

    # create and open RasterRow objects for classification and probabilities if enabled
    if class_probabilities == True and mode == 'classification':
        prob_out_raster = [0] * nclasses
        prob = [0] * nclasses
        for iclass in range(nclasses):
            prob_out_raster[iclass] = output + '_p' + str(class_list[iclass])
            prob[iclass] = RasterRow(prob_out_raster[iclass])
            prob[iclass].open('w', 'FCELL', overwrite=True)

    for rowblock in range(0, current.rows, rowincr):
        # check that the row increment does not exceed the number of rows
        if rowblock+rowincr > current.rows: rowincr = current.rows - rowblock
        img_np_row = np.zeros((rowincr, current.cols, nbands))
        mask_np_row = np.zeros((rowincr, current.cols))

        # loop through each row, and each band and add these values to the 2D array img_np_row
        for row in range(rowblock, rowblock+rowincr, 1):
            mask_np_row[row-rowblock, :] = np.array(mask_raster[row])
            for band in range(nbands):
                img_np_row[row-rowblock, :, band] = np.array(rasstack[band][row])

        mask_np_row[mask_np_row == -2147483648] = np.nan
        nanmask = np.isnan(mask_np_row) # True in the mask means invalid data

        # reshape each row-band matrix into a list
        nsamples = rowincr * current.cols
        flat_pixels = img_np_row.reshape((nsamples, nbands))

        # remove NaN values and perform the prediction
        flat_pixels_noNaN = np.nan_to_num(flat_pixels)
        result = rf.predict(flat_pixels_noNaN)
        result = result.reshape((rowincr, current.cols))

        # replace NaN values so that the prediction surface does not have a border
        result_NaN = np.ma.masked_array(result, mask=nanmask, fill_value=np.nan)

        # return a copy of result, with masked values filled with a given value
        result_masked = result_NaN.filled([nodata])

        # for each row we can perform computation, and write the result into
        for row in range(rowincr):
            newrow = Buffer((result_masked.shape[1],), mtype=ftype)
            newrow[:] = result_masked[row, :]
            classification.put_row(newrow)

        # same for probabilities
        if class_probabilities == True and mode == 'classification':
            result_proba = rf.predict_proba(flat_pixels_noNaN)
            for iclass in range(nclasses):
                result_proba_class = result_proba[:, iclass]
                result_proba_class = result_proba_class.reshape((rowincr, current.cols))
                result_proba_class_NaN = np.ma.masked_array(result_proba_class, mask=nanmask, fill_value=np.nan)
                result_proba_class_masked = result_proba_class_NaN.filled([np.nan])
                for row in range(rowincr):
                    newrow = Buffer((result_proba_class_masked.shape[1],), mtype='FCELL')
                    newrow[:] = result_proba_class_masked[row, :]
                    prob[iclass].put_row(newrow)

    classification.close()

    if class_probabilities == True and mode == 'classification':
        for iclass in range(nclasses): prob[iclass].close()

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
