#!/bin/bash

#m_orders=(1 2 3 4 5)
minus_log_eta=(0 1 2 3 4 5)
#cd ../../../
#configure --disable-vec --enable-no-gravity-below-id=11
#make -j 8
#cd -
for i in "${minus_log_eta[@]}"; do 
    cd minus_log_eta_$i
    python makeIC.py 10
    ../../../swift -G -e -t 1 particle_line.yml
    python energy_plot.py 11 $i
    rm *.hdf5
    cd ..
done
python eta_test_plot.py
echo "Done"



