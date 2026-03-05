


unsigned int
Data2D_Base::computeMinMaxK() const 
{
     double old_min_k = _min_k;
     double old_max_k = _max_k;
     unsigned int invalid_pixels = 0;
    _min_k = FLT_MAX;
    _max_k = -FLT_MAX;

#ifdef ENABLE_OPENMP
#define PAD 8
    // pad length = number of double precision words in a cache line
    // this spreads write operations across a larger area

    unsigned int numThreads = omp_get_max_threads(); //when in the loop, this will equal omp_get_num_threads();
    double minArray[numThreads][PAD];
    double maxArray[numThreads][PAD];
    unsigned int invalidArray[numThreads][PAD];


    for(unsigned int counter = 0; counter < numThreads; counter++){
        minArray[counter][0] = FLT_MAX;
        maxArray[counter][0] = -FLT_MAX;
        invalidArray[counter][0] = 0;
    }

// this conditional is a threshold operator; if the input array is too small to benefit from parallelism, a segfault can also occur. This makes sure that there is an arbitrary cut-off (some multiple of numThreads). Again, the cut-off choice is largely arbitrary but should be some small multiple of the number of threads in order to ensure there is a speed-up and not a segfault
if(numLines() < numThreads*10){
    for (unsigned int j = 0; j < numLines(); j++)
     {
      
         for (unsigned int i = 0; i < numSamples(); i++)
         {
             double value = static_cast<double>(valueAtPixel(i, j));

             if (isValidData(value))
             {
                 _min_k = (value < _min_k) ? value : _min_k;
                 _max_k = (value > _max_k) ? value : _max_k;
             }
             else
                 ++invalid_pixels;
         }
     }
}

    else{
        // note that this does NOT use omp parallel for, as that creates race conditions since this needs to compare among lines and between threads, rather than just process things line-wise
#pragma omp parallel num_threads(numThreads)
        for (unsigned int j = omp_get_thread_num(); j < numLines(); j = j + numThreads)
        {

            unsigned int threadNum = omp_get_thread_num(); 
     
            for (unsigned int i = 0; i < numSamples(); i++)
            {
                double value = static_cast<double>(valueAtPixel(i, j));
            
                if (isValidData(value)) //value != NO_DATA_VALUE)
                {
                minArray[threadNum][0] = (value < minArray[threadNum][0]) ? value : minArray[threadNum][0];
                maxArray[threadNum][0] = (value > maxArray[threadNum][0]) ? value : maxArray[threadNum][0];
                }
                else
                {
                    ++invalidArray[threadNum][0];
                }
            }


        }

// post-process step:
        double globalmin = FLT_MAX;
        double globalmax = -FLT_MAX; 
    

        for(unsigned int counter = 0; counter < numThreads; counter++){

            globalmin = (minArray[counter][0] < globalmin) ? minArray[counter][0] : globalmin;
            globalmax = (maxArray[counter][0] > globalmax) ? maxArray[counter][0] : globalmax;

        }
    
        _min_k = globalmin; 
        _max_k = globalmax;

        for(unsigned int counter = 0; counter < numThreads; counter++){

            invalid_pixels += invalidArray[counter][0];

        }
    }

#else
    // the normal subroutine, for backwards compatibility/if OpenMP is not enabled
     for (unsigned int j = 0; j < numLines(); j++)
     {
      
         for (unsigned int i = 0; i < numSamples(); i++)
         {
             double value = static_cast<double>(valueAtPixel(i, j));

             if (isValidData(value))
             {
                 _min_k = (value < _min_k) ? value : _min_k;
                 _max_k = (value > _max_k) ? value : _max_k;
             }
             else
                 ++invalid_pixels;
         }
     }
#endif



    // did not find any valid data in this Data2D
    // need a way to distinguish lack of data from min/max k values being uninitialized
    // set min to NO_DATA_VALUE,  and max == -NO_DATA_VALUE to indicate initialized but
    // no data at all

    // common end with non-openmp version:
    if (_min_k == FLT_MAX) 
      {
        _min_k = noDataValue();
        _max_k = -noDataValue();
      }



    if ((_min_k != old_min_k) or (_max_k != old_max_k))
        _locallyModifiedFlag = true;

    return invalid_pixels;
}