# BSD 3-Clause License
#
# Copyright (c) 2022, Arm Limited
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
param(
    [Parameter()]
    [UInt32]$nloops=1
)

for($i=0; $i -lt $nloops;$i++)
{
    wperf list
    wperf test
    wperf stat -m dcache,icache,imix -c 0,1,2,3,4 sleep 1
    wperf stat -m dcache,icache,imix -c 0,1,2,3 sleep 1
    wperf stat -m dcache,icache,imix -c 0,1,2 sleep 1
    wperf stat -m dcache,icache,imix -c 0,1 sleep 1
    wperf stat -m dcache,icache,imix -c 0 sleep 1

    wperf stat -m dcache,icache,imix -c 0,1,2,3,4 sleep 1   --json
    wperf stat -m dcache,icache,imix -c 0,1,2,3 sleep 1     --json
    wperf stat -m dcache,icache,imix -c 0,1,2 sleep 1       --json
    wperf stat -m dcache,icache,imix -c 0,1 sleep 1         --json
    wperf stat -m dcache,icache,imix -c 0 sleep 1           --json

    wperf stat -m imix -c 0,1,2,3,4 sleep 1   --json
    wperf stat -m dcache -c 0,1,2,3 sleep 1     --json
    wperf stat -m dcache,icache -c 0,1,2 sleep 1       --json
    wperf stat -m dcache,icache,imix -c 0,1 sleep 1         --json
    wperf stat -m dcache,icache,imix -c 0 sleep 1           --json
    wperf stat -m dcache,icache -c 0 sleep 1           --json
    wperf stat -m dcache -c 0 sleep 1           --json


    wperf list
    wperf test
    wperf stat -m dcache,icache,imix -c 0,1            sleep 10  --json
    wperf stat -m imix                                 sleep 10  --json

    wperf stat --output _output_01.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 sleep 5
    wperf stat --output _output_02.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 0 sleep 5
    cmd.exe /c "wperf stat --output _output_03.json -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec -c 0 sleep 5"
    wperf stat --output _output_04.json -m imix -e l1i_cache -c 0 sleep 5
    wperf stat --output _output_07.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0 sleep 5
    wperf stat --output _output_08.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 7 sleep 5
    cmd.exe /c "wperf stat --output _output_09.json -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec -c 6 sleep 5"
    wperf stat --output _output_10.json -m imix -e l1i_cache -c 2 sleep 5
    wperf stat --output _output_13.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec sleep 5
    wperf stat --output _output_14.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec sleep 5
    cmd.exe /c "wperf stat --output _output_15.json -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec sleep 5"
    wperf stat --output _output_16.json -m imix -e l1i_cache sleep 5
    wperf stat --output _output_19.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec -c 0,1,2,3,4 sleep 5
    wperf stat --output _output_20.json -e inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec,br_immed_spec,crypto_spec -c 1,3,5,7 sleep 5
    cmd.exe /c "wperf stat --output _output_21.json -e {inst_spec,vfp_spec,ase_spec,dp_spec,ld_spec,st_spec},br_immed_spec,crypto_spec -c 0,1,1,2,3,5 sleep 5"
    wperf stat --output _output_22.json -m imix -e l1i_cache -c 2,1,3,4,7,11,18,29,47,76 sleep 5

    python -m pytest
}