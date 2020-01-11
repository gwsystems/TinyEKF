#!/bin/sh

ITERS=$1

# before running this benchmark,
# copy fibonacci to fibonacci_native.out

testeach()
{
	tmp_cnt=${ITERS}

	while [ ${tmp_cnt} -gt 0 ]; do
		./gps_ekf_fn <ekf_raw.dat 1>out.dat
		tmp_cnt=$((tmp_cnt - 1))
	done

	echo "Done!"
}

testeach 
