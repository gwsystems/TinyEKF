#!/bin/bash
if (($# < 1)); then
  echo "Usage: $0 count"
  exit 1
fi

ITERS=$1

testeach() {
  tmp_cnt=${ITERS}

  while [ "${tmp_cnt}" -gt 0 ]; do
    ./gps_ekf_fn.out <ekf_raw.dat 1>out.dat
    tmp_cnt=$((tmp_cnt - 1))
  done

  echo "Done!"
}

testeach
