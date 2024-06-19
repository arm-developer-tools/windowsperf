#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2024, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
// DSU System Registers
//
#define CLUSTERPMCR_EL1             ARM64_SYSREG(1, 0, 15, 5, 0)
#define CLUSTERPMCNTENSET_EL1       ARM64_SYSREG(1, 0, 15, 5, 1)
#define CLUSTERPMCNTENCLR_EL1       ARM64_SYSREG(1, 0, 15, 5, 2)
#define CLUSTERPMSELR_EL1           ARM64_SYSREG(1, 0, 15, 5, 5)
#define CLUSTERPMCCNTR_EL1          ARM64_SYSREG(1, 0, 15, 6, 0)
#define CLUSTERPMXEVTYPER_EL1       ARM64_SYSREG(1, 0, 15, 6, 1)
#define CLUSTERPMXEVCNTR_EL1        ARM64_SYSREG(1, 0, 15, 6, 2)
#define CLUSTERPMCEID0_EL1          ARM64_SYSREG(1, 0, 15, 6, 4)
#define CLUSTERPMCEID1_EL1          ARM64_SYSREG(1, 0, 15, 6, 5)

//
// System Registers 
//
#define PMCNTENSET_EL0				ARM64_SYSREG(1, 3, 9, 12, 1)
#define PMCNTENCLR_EL0				ARM64_SYSREG(1, 3, 9, 12, 2)
#define PMINTENSET_EL1				ARM64_SYSREG(1, 0, 9, 14, 1)
#define PMINTENCLR_EL1				ARM64_SYSREG(1, 0, 9, 14, 2)
#define PMOVSCLR_EL0				ARM64_SYSREG(1, 3, 9, 12, 3)
#define PMCR_EL0					ARM64_SYSREG(1, 3, 9, 12, 0)
#define PMCCFILTR_EL0				ARM64_SYSREG(1, 3, 14,15, 7)
#define PMCCNTR_EL0					ARM64_SYSREG(1, 3, 9, 13, 0)
#define ID_DFR0_EL1					ARM64_SYSREG(1, 0, 0,  5, 0)
#define MIDR_EL1					ARM64_SYSREG(1, 0, 0,  0, 0)
#define ID_AA64DFR0_EL1				ARM64_SYSREG(3, 0, 0,  5, 0)

#define PMEVTYPER0_EL0				ARM64_SYSREG(1, 3, 14, 12, 0)
#define PMEVTYPER1_EL0				ARM64_SYSREG(1, 3, 14, 12, 1)
#define PMEVTYPER2_EL0				ARM64_SYSREG(1, 3, 14, 12, 2)
#define PMEVTYPER3_EL0				ARM64_SYSREG(1, 3, 14, 12, 3)
#define PMEVTYPER4_EL0				ARM64_SYSREG(1, 3, 14, 12, 4)
#define PMEVTYPER5_EL0				ARM64_SYSREG(1, 3, 14, 12, 5)
#define PMEVTYPER6_EL0				ARM64_SYSREG(1, 3, 14, 12, 6)
#define PMEVTYPER7_EL0				ARM64_SYSREG(1, 3, 14, 12, 7)
#define PMEVTYPER8_EL0				ARM64_SYSREG(1, 3, 14, 13, 0)
#define PMEVTYPER9_EL0				ARM64_SYSREG(1, 3, 14, 13, 1)
#define PMEVTYPER10_EL0				ARM64_SYSREG(1, 3, 14, 13, 2)
#define PMEVTYPER11_EL0				ARM64_SYSREG(1, 3, 14, 13, 3)
#define PMEVTYPER12_EL0				ARM64_SYSREG(1, 3, 14, 13, 4)
#define PMEVTYPER13_EL0				ARM64_SYSREG(1, 3, 14, 13, 5)
#define PMEVTYPER14_EL0				ARM64_SYSREG(1, 3, 14, 13, 6)
#define PMEVTYPER15_EL0				ARM64_SYSREG(1, 3, 14, 13, 7)
#define PMEVTYPER16_EL0				ARM64_SYSREG(1, 3, 14, 14, 0)
#define PMEVTYPER17_EL0				ARM64_SYSREG(1, 3, 14, 14, 1)
#define PMEVTYPER18_EL0				ARM64_SYSREG(1, 3, 14, 14, 2)
#define PMEVTYPER19_EL0				ARM64_SYSREG(1, 3, 14, 14, 3)
#define PMEVTYPER20_EL0				ARM64_SYSREG(1, 3, 14, 14, 4)
#define PMEVTYPER21_EL0				ARM64_SYSREG(1, 3, 14, 14, 5)
#define PMEVTYPER22_EL0				ARM64_SYSREG(1, 3, 14, 14, 6)
#define PMEVTYPER23_EL0				ARM64_SYSREG(1, 3, 14, 14, 7)
#define PMEVTYPER24_EL0				ARM64_SYSREG(1, 3, 14, 15, 0)
#define PMEVTYPER25_EL0				ARM64_SYSREG(1, 3, 14, 15, 1)
#define PMEVTYPER26_EL0				ARM64_SYSREG(1, 3, 14, 15, 2)
#define PMEVTYPER27_EL0				ARM64_SYSREG(1, 3, 14, 15, 3)
#define PMEVTYPER28_EL0				ARM64_SYSREG(1, 3, 14, 15, 4)
#define PMEVTYPER29_EL0				ARM64_SYSREG(1, 3, 14, 15, 5)
#define PMEVTYPER30_EL0				ARM64_SYSREG(1, 3, 14, 15, 6)

#define PMEVCNTR0_EL0				ARM64_SYSREG(1, 3, 14, 8, 0)
#define PMEVCNTR1_EL0				ARM64_SYSREG(1, 3, 14, 8, 1)
#define PMEVCNTR2_EL0				ARM64_SYSREG(1, 3, 14, 8, 2)
#define PMEVCNTR3_EL0				ARM64_SYSREG(1, 3, 14, 8, 3)
#define PMEVCNTR4_EL0				ARM64_SYSREG(1, 3, 14, 8, 4)
#define PMEVCNTR5_EL0				ARM64_SYSREG(1, 3, 14, 8, 5)
#define PMEVCNTR6_EL0				ARM64_SYSREG(1, 3, 14, 8, 6)
#define PMEVCNTR7_EL0				ARM64_SYSREG(1, 3, 14, 8, 7)
#define PMEVCNTR8_EL0				ARM64_SYSREG(1, 3, 14, 9, 0)
#define PMEVCNTR9_EL0				ARM64_SYSREG(1, 3, 14, 9, 1)
#define PMEVCNTR10_EL0				ARM64_SYSREG(1, 3, 14, 9, 2)
#define PMEVCNTR11_EL0				ARM64_SYSREG(1, 3, 14, 9, 3)
#define PMEVCNTR12_EL0				ARM64_SYSREG(1, 3, 14, 9, 4)
#define PMEVCNTR13_EL0				ARM64_SYSREG(1, 3, 14, 9, 5)
#define PMEVCNTR14_EL0				ARM64_SYSREG(1, 3, 14, 9, 6)
#define PMEVCNTR15_EL0				ARM64_SYSREG(1, 3, 14, 9, 7)
#define PMEVCNTR16_EL0				ARM64_SYSREG(1, 3, 14, 10, 0)
#define PMEVCNTR17_EL0				ARM64_SYSREG(1, 3, 14, 10, 1)
#define PMEVCNTR18_EL0				ARM64_SYSREG(1, 3, 14, 10, 2)
#define PMEVCNTR19_EL0				ARM64_SYSREG(1, 3, 14, 10, 3)
#define PMEVCNTR20_EL0				ARM64_SYSREG(1, 3, 14, 10, 4)
#define PMEVCNTR21_EL0				ARM64_SYSREG(1, 3, 14, 10, 5)
#define PMEVCNTR22_EL0				ARM64_SYSREG(1, 3, 14, 10, 6)
#define PMEVCNTR23_EL0				ARM64_SYSREG(1, 3, 14, 10, 7)
#define PMEVCNTR24_EL0				ARM64_SYSREG(1, 3, 14, 11, 0)
#define PMEVCNTR25_EL0				ARM64_SYSREG(1, 3, 14, 11, 1)
#define PMEVCNTR26_EL0				ARM64_SYSREG(1, 3, 14, 11, 2)
#define PMEVCNTR27_EL0				ARM64_SYSREG(1, 3, 14, 11, 3)
#define PMEVCNTR28_EL0				ARM64_SYSREG(1, 3, 14, 11, 4)
#define PMEVCNTR29_EL0				ARM64_SYSREG(1, 3, 14, 11, 5)
#define PMEVCNTR30_EL0				ARM64_SYSREG(1, 3, 14, 11, 6)

// SPE Registers from https://developer.arm.com/documentation/100616/0400/debug-registers/spe-registers/spe-register-summary?lang=en
// 																	 Op0	Op1	CRn	CRm	Op2	Name			Type	Reset			Description
#define PMSCR_EL1					ARM64_SYSREG(3, 0, 9, 9,	0) //3		0	c9	c9	0	PMSCR_EL1		RW		UNK				Statistical Profiling Control Register EL1
#define PMSCR_EL2					ARM64_SYSREG(3, 4, 9, 9,	0) //3		4	c9	c9	0	PMSCR_EL2		RW		UNK				Statistical Profiling Control Register EL2
#define PMSCR_EL12					ARM64_SYSREG(3, 5, 9, 9,	0) //3		5	c9	c9	0	PMSCR_EL12		RW		UNK				Alias of the PMSCR_EL1 register, available in EL2
#define PMSICR_EL1					ARM64_SYSREG(3, 0, 9, 9,	2) //3		0	c9	c9	2	PMSICR_EL1		RW		UNK				Sampling Interval Counter Register
#define PMSIRR_EL1					ARM64_SYSREG(3, 0, 9, 9,	3) //3		0	c9	c9	3	PMSIRR_EL1		RW		UNK				Sampling Interval Reload Register
#define PMSEVFR_EL1					ARM64_SYSREG(3, 0, 9, 9,	5) //3		0	c9	c9	5	PMSEVFR_EL1		RW		UNK				Sampling Event Filter Register
#define PMSLATFR_EL1				ARM64_SYSREG(3, 0, 9, 9,	6) //3		0	c9	c9	6	PMSLATFR_EL1	RW		UNK				Sampling Latency Filter Register
#define PMBPTR_EL1					ARM64_SYSREG(3, 0, 9, 10,	1) //3		0	c9	c10	1	PMBPTR_EL1		RW		UNK				Profiling Buffer Write Pointer Register
#define PMBLIMITR_EL1				ARM64_SYSREG(3, 0, 9, 10,	0) //3		0	c9	c10	0	PMBLIMITR_EL1	RW		00000000		Profiling Buffer Limit Address Register
#define PMBSR_EL1					ARM64_SYSREG(3, 0, 9, 10,	3) //3		0	c9	c10	3	PMBSR_EL1		RW		UNK				Profiling Buffer Status / syndrome Register
#define PMSFCR_EL1					ARM64_SYSREG(3, 0, 9, 9,	4) //3		0	c9	c9	4	PMSFCR_EL1		RW		UNK	Sampling	Filter Control Register
#define PMBIDR_EL1					ARM64_SYSREG(3, 0, 9, 10,	7) //3		0	c9	c10	7	PMBIDR_EL1		RO		00000026		Profiling Buffer ID Register
#define PMSIDR_EL1					ARM64_SYSREG(3, 0, 9, 9,	7) //3		0	c9	c9	7	PMSIDR_EL1		RO		00026497		Sampling Profiling ID Register
