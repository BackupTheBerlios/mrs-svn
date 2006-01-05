/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "MRS.h"

#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

#include "HError.h"

#include "CMatrix.h"
//#include "utils.h"

using namespace std;

struct MatrixInfo
{
	const char*	name;
	short		gap_open;
	short		gap_extend;
	short		dummy;
	double		lambda;
	double		kappa;
	double		entropy;
	double		alpha;
	double		beta;
};

const unsigned long kMaxShort = numeric_limits<short>::max();

const MatrixInfo kMatrix[] = {
    {"BLOSUM45", kMaxShort, kMaxShort, kMaxShort, 0.2291, 0.0924, 0.2514, 0.9113, -5.7},
    {"BLOSUM45", 13, 3, kMaxShort, 0.207, 0.049, 0.14, 1.5, -22},
    {"BLOSUM45", 12, 3, kMaxShort, 0.199, 0.039, 0.11, 1.8, -34},
    {"BLOSUM45", 11, 3, kMaxShort, 0.190, 0.031, 0.095, 2.0, -38},
    {"BLOSUM45", 10, 3, kMaxShort, 0.179, 0.023, 0.075, 2.4, -51},
    {"BLOSUM45", 16, 2, kMaxShort, 0.210, 0.051, 0.14, 1.5, -24},
    {"BLOSUM45", 15, 2, kMaxShort, 0.203, 0.041, 0.12, 1.7, -31},
    {"BLOSUM45", 14, 2, kMaxShort, 0.195, 0.032, 0.10, 1.9, -36},
    {"BLOSUM45", 13, 2, kMaxShort, 0.185, 0.024, 0.084, 2.2, -45},
    {"BLOSUM45", 12, 2, kMaxShort, 0.171, 0.016, 0.061, 2.8, -65},
    {"BLOSUM45", 19, 1, kMaxShort, 0.205, 0.040, 0.11, 1.9, -43},
    {"BLOSUM45", 18, 1, kMaxShort, 0.198, 0.032, 0.10, 2.0, -43},
    {"BLOSUM45", 17, 1, kMaxShort, 0.189, 0.024, 0.079, 2.4, -57},
    {"BLOSUM45", 16, 1, kMaxShort, 0.176, 0.016, 0.063, 2.8, -67},
    {"BLOSUM50", kMaxShort, kMaxShort, kMaxShort, 0.2318, 0.112, 0.3362, 0.6895, -4.0},
    {"BLOSUM50", 13, 3, kMaxShort, 0.212, 0.063, 0.19, 1.1, -16},
    {"BLOSUM50", 12, 3, kMaxShort, 0.206, 0.055, 0.17, 1.2, -18},
    {"BLOSUM50", 11, 3, kMaxShort, 0.197, 0.042, 0.14, 1.4, -25},
    {"BLOSUM50", 10, 3, kMaxShort, 0.186, 0.031, 0.11, 1.7, -34},
    {"BLOSUM50",  9, 3, kMaxShort, 0.172, 0.022, 0.082, 2.1, -48},
    {"BLOSUM50", 16, 2, kMaxShort, 0.215, 0.066, 0.20, 1.05, -15},
    {"BLOSUM50", 15, 2, kMaxShort, 0.210, 0.058, 0.17, 1.2, -20},
    {"BLOSUM50", 14, 2, kMaxShort, 0.202, 0.045, 0.14, 1.4, -27},
    {"BLOSUM50", 13, 2, kMaxShort, 0.193, 0.035, 0.12, 1.6, -32},
    {"BLOSUM50", 12, 2, kMaxShort, 0.181, 0.025, 0.095, 1.9, -41},
    {"BLOSUM50", 19, 1, kMaxShort, 0.212, 0.057, 0.18, 1.2, -21},
    {"BLOSUM50", 18, 1, kMaxShort, 0.207, 0.050, 0.15, 1.4, -28},
    {"BLOSUM50", 17, 1, kMaxShort, 0.198, 0.037, 0.12, 1.6, -33},
    {"BLOSUM50", 16, 1, kMaxShort, 0.186, 0.025, 0.10, 1.9, -42},
    {"BLOSUM50", 15, 1, kMaxShort, 0.171, 0.015, 0.063, 2.7, -76},
    {"BLOSUM62", kMaxShort, kMaxShort, kMaxShort, 0.3176, 0.134, 0.4012, 0.7916, -3.2},
    {"BLOSUM62", 11, 2, kMaxShort, 0.297, 0.082, 0.27, 1.1, -10},
    {"BLOSUM62", 10, 2, kMaxShort, 0.291, 0.075, 0.23, 1.3, -15},
    {"BLOSUM62",  9, 2, kMaxShort, 0.279, 0.058, 0.19, 1.5, -19},
    {"BLOSUM62",  8, 2, kMaxShort, 0.264, 0.045, 0.15, 1.8, -26},
    {"BLOSUM62",  7, 2, kMaxShort, 0.239, 0.027, 0.10, 2.5, -46},
    {"BLOSUM62",  6, 2, kMaxShort, 0.201, 0.012, 0.061, 3.3, -58},
    {"BLOSUM62", 13, 1, kMaxShort, 0.292, 0.071, 0.23, 1.2, -11},
    {"BLOSUM62", 12, 1, kMaxShort, 0.283, 0.059, 0.19, 1.5, -19},
    {"BLOSUM62", 11, 1, kMaxShort, 0.267, 0.041, 0.14, 1.9, -30},
    {"BLOSUM62", 10, 1, kMaxShort, 0.243, 0.024, 0.10, 2.5, -44},
    {"BLOSUM62",  9, 1, kMaxShort, 0.206, 0.010, 0.052, 4.0, -87},
    {"BLOSUM80", kMaxShort, kMaxShort, kMaxShort, 0.3430, 0.177, 0.6568, 0.5222, -1.6},
    {"BLOSUM80", 25, 2, kMaxShort, 0.342, 0.17, 0.66, 0.52, -1.6},
    {"BLOSUM80", 13, 2, kMaxShort, 0.336, 0.15, 0.57, 0.59, -3},
    {"BLOSUM80",  9, 2, kMaxShort, 0.319, 0.11, 0.42, 0.76, -6},
    {"BLOSUM80",  8, 2, kMaxShort, 0.308, 0.090, 0.35, 0.89, -9},
    {"BLOSUM80",  7, 2, kMaxShort, 0.293, 0.070, 0.27, 1.1, -14},
    {"BLOSUM80",  6, 2, kMaxShort, 0.268, 0.045, 0.19, 1.4, -19},
    {"BLOSUM80", 11, 1, kMaxShort, 0.314, 0.095, 0.35, 0.90, -9},
    {"BLOSUM80", 10, 1, kMaxShort, 0.299, 0.071, 0.27, 1.1, -14},
    {"BLOSUM80",  9, 1, kMaxShort, 0.279, 0.048, 0.20, 1.4, -19},
    {"BLOSUM90", kMaxShort, kMaxShort, kMaxShort, 0.3346, 0.190, 0.7547, 0.4434, -1.4},
    {"BLOSUM90",  9, 2, kMaxShort, 0.310, 0.12, 0.46, 0.67, -6},
    {"BLOSUM90",  8, 2, kMaxShort, 0.300, 0.099, 0.39, 0.76, -7},
    {"BLOSUM90",  7, 2, kMaxShort, 0.283, 0.072, 0.30, 0.93, -11},
    {"BLOSUM90",  6, 2, kMaxShort, 0.259, 0.048, 0.22, 1.2, -16},
    {"BLOSUM90", 11, 1, kMaxShort, 0.302, 0.093, 0.39, 0.78, -8},
    {"BLOSUM90", 10, 1, kMaxShort, 0.290, 0.075, 0.28, 1.04, -15},
    {"BLOSUM90",  9, 1, kMaxShort, 0.265, 0.044, 0.20, 1.3, -19},
    {"PAM250", kMaxShort, kMaxShort, kMaxShort, 0.2252, 0.0868, 0.2223, 0.98, -5.0},
    {"PAM250", 15, 3, kMaxShort, 0.205, 0.049, 0.13, 1.6, -23},
    {"PAM250", 14, 3, kMaxShort, 0.200, 0.043, 0.12, 1.7, -26},
    {"PAM250", 13, 3, kMaxShort, 0.194, 0.036, 0.10, 1.9, -31},
    {"PAM250", 12, 3, kMaxShort, 0.186, 0.029, 0.085, 2.2, -41},
    {"PAM250", 11, 3, kMaxShort, 0.174, 0.020, 0.070, 2.5, -48},
    {"PAM250", 17, 2, kMaxShort, 0.204, 0.047, 0.12, 1.7, -28},
    {"PAM250", 16, 2, kMaxShort, 0.198, 0.038, 0.11, 1.8, -29},
    {"PAM250", 15, 2, kMaxShort, 0.191, 0.031, 0.087, 2.2, -44},
    {"PAM250", 14, 2, kMaxShort, 0.182, 0.024, 0.073, 2.5, -53},
    {"PAM250", 13, 2, kMaxShort, 0.171, 0.017, 0.059, 2.9, -64},
    {"PAM250", 21, 1, kMaxShort, 0.205, 0.045, 0.11, 1.8, -34},
    {"PAM250", 20, 1, kMaxShort, 0.199, 0.037, 0.10, 1.9, -35},
    {"PAM250", 19, 1, kMaxShort, 0.192, 0.029, 0.083, 2.3, -52},
    {"PAM250", 18, 1, kMaxShort, 0.183, 0.021, 0.070, 2.6, -60},
    {"PAM250", 17, 1, kMaxShort, 0.171, 0.014, 0.052, 3.3, -86},
    {"PAM30", kMaxShort, kMaxShort, kMaxShort, 0.3400, 0.283, 1.754, 0.1938, -0.3},
    {"PAM30", 7, 2, kMaxShort, 0.305, 0.15, 0.87, 0.35, -3},
    {"PAM30", 6, 2, kMaxShort, 0.287, 0.11, 0.68, 0.42, -4},
    {"PAM30", 5, 2, kMaxShort, 0.264, 0.079, 0.45, 0.59, -7},
    {"PAM30", 10, 1, kMaxShort, 0.309, 0.15, 0.88, 0.35, -3},
    {"PAM30",  9, 1, kMaxShort, 0.294, 0.11, 0.61, 0.48, -6},
    {"PAM30", 8, 1, kMaxShort, 0.270, 0.072, 0.40, 0.68, -10},
    {"PAM70", kMaxShort, kMaxShort, kMaxShort, 0.3345, 0.229, 1.029, 0.3250,   -0.7},
    {"PAM70", 8, 2, kMaxShort, 0.301, 0.12, 0.54, 0.56, -5},
    {"PAM70", 7, 2, kMaxShort, 0.286, 0.093, 0.43, 0.67, -7},
    {"PAM70", 6, 2, kMaxShort, 0.264, 0.064, 0.29, 0.90, -12},
    {"PAM70", 11, 1, kMaxShort, 0.305, 0.12, 0.52, 0.59, -6},
    {"PAM70",  10, 1, kMaxShort, 0.291, 0.091, 0.41, 0.71, -9},
    {"PAM70", 9, 1, kMaxShort, 0.270, 0.060, 0.28, 0.97, -14},
    {"BLOSUM62_20", kMaxShort, kMaxShort, kMaxShort, 0.03391, 0.125, 0.4544, 0.07462, -3.2},
    {"BLOSUM62_20", 100, 12, kMaxShort, 0.0300, 0.056, 0.21, 0.14, -15},
    {"BLOSUM62_20",  95, 12, kMaxShort, 0.0291, 0.047, 0.18, 0.16, -20},
    {"BLOSUM62_20",  90, 12, kMaxShort, 0.0280, 0.038, 0.15, 0.19, -28},
    {"BLOSUM62_20",  85, 12, kMaxShort, 0.0267, 0.030, 0.13, 0.21, -31},
    {"BLOSUM62_20",  80, 12, kMaxShort, 0.0250, 0.021, 0.10, 0.25, -39},
    {"BLOSUM62_20", 105, 11, kMaxShort, 0.0301, 0.056, 0.22, 0.14, -16},
    {"BLOSUM62_20", 100, 11, kMaxShort, 0.0294, 0.049, 0.20, 0.15, -17},
    {"BLOSUM62_20",  95, 11, kMaxShort, 0.0285, 0.042, 0.16, 0.18, -25},
    {"BLOSUM62_20",  90, 11, kMaxShort, 0.0271, 0.031, 0.14, 0.20, -28},
    {"BLOSUM62_20",  85, 11, kMaxShort, 0.0256, 0.023, 0.10, 0.26, -46},
    {"BLOSUM62_20", 115, 10, kMaxShort, 0.0308, 0.062, 0.22, 0.14, -20},
    {"BLOSUM62_20", 110, 10, kMaxShort, 0.0302, 0.056, 0.19, 0.16, -26},
    {"BLOSUM62_20", 105, 10, kMaxShort, 0.0296, 0.050, 0.17, 0.17, -27},
    {"BLOSUM62_20", 100, 10, kMaxShort, 0.0286, 0.041, 0.15, 0.19, -32},
    {"BLOSUM62_20",  95, 10, kMaxShort, 0.0272, 0.030, 0.13, 0.21, -35},
    {"BLOSUM62_20",  90, 10, kMaxShort, 0.0257, 0.022, 0.11, 0.24, -40},
    {"BLOSUM62_20",  85, 10, kMaxShort, 0.0242, 0.017, 0.083, 0.29, -51},
    {"BLOSUM62_20", 115, 9, kMaxShort, 0.0306, 0.061, 0.24, 0.13, -14},
    {"BLOSUM62_20", 110, 9, kMaxShort, 0.0299, 0.053, 0.19, 0.16, -23},
    {"BLOSUM62_20", 105, 9, kMaxShort, 0.0289, 0.043, 0.17, 0.17, -23},
    {"BLOSUM62_20", 100, 9, kMaxShort, 0.0279, 0.036, 0.14, 0.20, -31},
    {"BLOSUM62_20",  95, 9, kMaxShort, 0.0266, 0.028, 0.12, 0.23, -37},
    {"BLOSUM62_20", 120, 8, kMaxShort, 0.0307, 0.062, 0.22, 0.14, -18},
    {"BLOSUM62_20", 115, 8, kMaxShort, 0.0300, 0.053, 0.20, 0.15, -19},
    {"BLOSUM62_20", 110, 8, kMaxShort, 0.0292, 0.046, 0.17, 0.17, -23},
    {"BLOSUM62_20", 105, 8, kMaxShort, 0.0280, 0.035, 0.14, 0.20, -31},
    {"BLOSUM62_20", 100, 8, kMaxShort, 0.0266, 0.026, 0.12, 0.23, -37},
    {"BLOSUM62_20", 125, 7, kMaxShort, 0.0306, 0.058, 0.22, 0.14, -18},
    {"BLOSUM62_20", 120, 7, kMaxShort, 0.0300, 0.052, 0.19, 0.16, -23},
    {"BLOSUM62_20", 115, 7, kMaxShort, 0.0292, 0.044, 0.17, 0.17, -24},
    {"BLOSUM62_20", 110, 7, kMaxShort, 0.0279, 0.032, 0.14, 0.20, -31},
    {"BLOSUM62_20", 105, 7, kMaxShort, 0.0267, 0.026, 0.11, 0.24, -41},
    {"BLOSUM62_20", 120, 10,5, 0.0298, 0.049, 0.19, 0.16, -21},
    {"BLOSUM62_20", 115, 10,5, 0.0290, 0.042, 0.16, 0.18, -25},
    {"BLOSUM62_20", 110, 10,5, 0.0279, 0.033, 0.13, 0.21, -32},
    {"BLOSUM62_20", 105, 10,5, 0.0264, 0.024, 0.10, 0.26, -46},
    {"BLOSUM62_20", 100, 10,5, 0.0250, 0.018, 0.081, 0.31, -56},
    {"BLOSUM62_20", 125, 10,4, 0.0301, 0.053, 0.18, 0.17, -25},
    {"BLOSUM62_20", 120, 10,4, 0.0292, 0.043, 0.15, 0.20, -33},
    {"BLOSUM62_20", 115, 10,4, 0.0282, 0.035, 0.13, 0.22, -36},
    {"BLOSUM62_20", 110, 10,4, 0.0270, 0.027, 0.11, 0.25, -41},
    {"BLOSUM62_20", 105, 10,4, 0.0254, 0.020, 0.079, 0.32, -60},
    {"BLOSUM62_20", 130, 10,3, 0.0300, 0.051, 0.17, 0.18, -27},
    {"BLOSUM62_20", 125, 10,3, 0.0290, 0.040, 0.13, 0.22, -38},
    {"BLOSUM62_20", 120, 10,3, 0.0278, 0.030, 0.11, 0.25, -44},
    {"BLOSUM62_20", 115, 10,3, 0.0267, 0.025, 0.092, 0.29, -52},
    {"BLOSUM62_20", 110, 10,3, 0.0252, 0.018, 0.070, 0.36, -70},
    {"BLOSUM62_20", 135, 10,2, 0.0292, 0.040, 0.13, 0.22, -35},
    {"BLOSUM62_20", 130, 10,2, 0.0283, 0.034, 0.10, 0.28, -51},
    {"BLOSUM62_20", 125, 10,2, 0.0269, 0.024, 0.077, 0.35, -71},
    {"BLOSUM62_20", 120, 10,2, 0.0253, 0.017, 0.059, 0.43, -90},
    {"BLOSUM62_20", 115, 10,2, 0.0234, 0.011, 0.043, 0.55, -121},
    {"BLOSUM62_20", 100, 14,3, 0.0258, 0.023, 0.087, 0.33, -59},
    {"BLOSUM62_20", 105, 13,3, 0.0263, 0.024, 0.085, 0.31, -57},
    {"BLOSUM62_20", 110, 12,3, 0.0271, 0.028, 0.093, 0.29, -54},
    {"BLOSUM62_20", 115, 11,3, 0.0275, 0.030, 0.10, 0.27, -49},
    {"BLOSUM62_20", 125, 9,3, 0.0283, 0.034, 0.12, 0.23, -38},
    {"BLOSUM62_20", 130, 8,3, 0.0287, 0.037, 0.12, 0.23, -40},
    {"BLOSUM62_20", 125, 7,3, 0.0287, 0.036, 0.12, 0.24, -44},
    {"BLOSUM62_20", 140, 6,3, 0.0285, 0.033, 0.12, 0.23, -40},
    {"BLOSUM62_20", 105, 14,3, 0.0270, 0.028, 0.10, 0.27, -46},
    {"BLOSUM62_20", 110, 13 ,3, 0.0279, 0.034, 0.10, 0.27, -50},
    {"BLOSUM62_20", 115, 12,3, 0.0282, 0.035, 0.12, 0.24, -42},
    {"BLOSUM62_20", 120, 11,3, 0.0286, 0.037, 0.12, 0.24, -44},
    { 0 }
};

const char kBlosum62Data[] =
	"#  Matrix made by matblas from blosum62.iij \n"
	"#  * column uses minimum score\n"
	"#  BLOSUM Clustered Scoring Matrix in 1/2 Bit Units\n"
	"#  Blocks Database = /data/blocks_5.0/blocks.dat\n"
	"#  Cluster Percentage: >= 62\n"
	"#  Entropy =   0.6979, Expected =  -0.5209\n"
	"   A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  Z  X  *\n"
	"A  4 -1 -2 -2  0 -1 -1  0 -2 -1 -1 -1 -1 -2 -1  1  0 -3 -2  0 -2 -1  0 -4 \n"
	"R -1  5  0 -2 -3  1  0 -2  0 -3 -2  2 -1 -3 -2 -1 -1 -3 -2 -3 -1  0 -1 -4 \n"
	"N -2  0  6  1 -3  0  0  0  1 -3 -3  0 -2 -3 -2  1  0 -4 -2 -3  3  0 -1 -4 \n"
	"D -2 -2  1  6 -3  0  2 -1 -1 -3 -4 -1 -3 -3 -1  0 -1 -4 -3 -3  4  1 -1 -4 \n"
	"C  0 -3 -3 -3  9 -3 -4 -3 -3 -1 -1 -3 -1 -2 -3 -1 -1 -2 -2 -1 -3 -3 -2 -4 \n"
	"Q -1  1  0  0 -3  5  2 -2  0 -3 -2  1  0 -3 -1  0 -1 -2 -1 -2  0  3 -1 -4 \n"
	"E -1  0  0  2 -4  2  5 -2  0 -3 -3  1 -2 -3 -1  0 -1 -3 -2 -2  1  4 -1 -4 \n"
	"G  0 -2  0 -1 -3 -2 -2  6 -2 -4 -4 -2 -3 -3 -2  0 -2 -2 -3 -3 -1 -2 -1 -4 \n"
	"H -2  0  1 -1 -3  0  0 -2  8 -3 -3 -1 -2 -1 -2 -1 -2 -2  2 -3  0  0 -1 -4 \n"
	"I -1 -3 -3 -3 -1 -3 -3 -4 -3  4  2 -3  1  0 -3 -2 -1 -3 -1  3 -3 -3 -1 -4 \n"
	"L -1 -2 -3 -4 -1 -2 -3 -4 -3  2  4 -2  2  0 -3 -2 -1 -2 -1  1 -4 -3 -1 -4 \n"
	"K -1  2  0 -1 -3  1  1 -2 -1 -3 -2  5 -1 -3 -1  0 -1 -3 -2 -2  0  1 -1 -4 \n"
	"M -1 -1 -2 -3 -1  0 -2 -3 -2  1  2 -1  5  0 -2 -1 -1 -1 -1  1 -3 -1 -1 -4 \n"
	"F -2 -3 -3 -3 -2 -3 -3 -3 -1  0  0 -3  0  6 -4 -2 -2  1  3 -1 -3 -3 -1 -4 \n"
	"P -1 -2 -2 -1 -3 -1 -1 -2 -2 -3 -3 -1 -2 -4  7 -1 -1 -4 -3 -2 -2 -1 -2 -4 \n"
	"S  1 -1  1  0 -1  0  0  0 -1 -2 -2  0 -1 -2 -1  4  1 -3 -2 -2  0  0  0 -4 \n"
	"T  0 -1  0 -1 -1 -1 -1 -2 -2 -1 -1 -1 -1 -2 -1  1  5 -2 -2  0 -1 -1  0 -4 \n"
	"W -3 -3 -4 -4 -2 -2 -3 -2 -2 -3 -2 -3 -1  1 -4 -3 -2 11  2 -3 -4 -3 -2 -4 \n"
	"Y -2 -2 -2 -3 -2 -1 -2 -3  2 -1 -1 -2 -1  3 -3 -2 -2  2  7 -1 -3 -2 -1 -4 \n"
	"V  0 -3 -3 -3 -1 -2 -2 -3 -3  3  1 -2  1 -1 -2 -2  0 -3 -1  4 -3 -2 -1 -4 \n"
	"B -2 -1  3  4 -3  0  1 -1  0 -3 -4  0 -3 -3 -2  0 -1 -4 -3 -3  4  1 -1 -4 \n"
	"Z -1  0  0  1 -3  3  4 -2  0 -3 -3  1 -1 -3 -1  0 -1 -3 -2 -2  1  4 -1 -4 \n"
	"X  0 -1 -1 -1 -2 -1 -1 -1 -1 -1 -1 -1 -1 -1 -2  0  0 -2 -1 -1 -1 -1 -1 -4 \n"
	"* -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4 -4  1 \n";

CMatrix::CMatrix(const string& inName, long inGapOpen, long inGapExtend)
{
	// for now....
	
	const MatrixInfo* minfo;
	
	for (minfo = kMatrix; minfo->name != NULL; ++minfo)
	{
		if (inName == minfo->name and
			inGapOpen == minfo->gap_open and
			inGapExtend == minfo->gap_extend)
		{
			break;
		}
	}
	
	if (minfo->name == NULL)
		THROW(("Unsupported matrix: %s", inName.c_str()));
	
	gapped.lambda = minfo->lambda;
	gapped.K = minfo->kappa;
	gapped.logK = log(gapped.K);
	gapped.H = minfo->entropy;
	gapped.alpha = minfo->alpha;
	gapped.beta = minfo->beta;

	for (minfo = kMatrix; minfo->name != NULL; ++minfo)
	{
		if (inName == minfo->name and
			kMaxShort == minfo->gap_open and
			kMaxShort == minfo->gap_extend)
		{
			break;
		}
	}
	
	if (minfo->name == NULL)
		THROW(("Unsupported matrix: %s", inName.c_str()));
	
	ungapped.lambda = minfo->lambda;
	ungapped.K = minfo->kappa;
	ungapped.logK = log(ungapped.K);
	ungapped.H = minfo->entropy;
	ungapped.alpha = minfo->alpha;
	ungapped.beta = minfo->beta;
	
	if (inName == "BLOSUM62")
	{
		istringstream s(kBlosum62Data);
		Read(s);
	}
	else
	{
		ifstream in(inName.c_str());

		if (not in.is_open())
			THROW(("Could not open matrix file for %s", inName.c_str()));
		
		Read(in);
	}
}		

void CMatrix::Read(istream& in)
{
	mLowestScore = mHighestScore = 0;
	
	for (int x = 0; x < kAACodeCount; ++x)
	{
		mTable[x] = mTableData + x * kAACodeCount;
		
		for (int y = 0; y < kAACodeCount; ++y)
			mTable[x][y] = kSentinalScore;
	}
	
	char columnHeadings[kAACodeCount] = { 0 };
	int lineNumber = 0;
	
	while (not in.eof())
	{
		string line;
		getline(in, line);
		
		if (line.length() == 0 or line[0] == '#')
			continue;

		stringstream ss(line);
		
		if (lineNumber == 0)
		{
			for (int i = 0; i < kAACodeCount and not ss.eof(); ++i)
				ss >> columnHeadings[i];
		}
		else
		{
			char row;
			ss >> row;
			
			for (int i = 0; i < kAACodeCount and not ss.eof(); ++i)
			{
				int32 score;
				ss >> score;
				
				int x = Encode(row);
				int y = Encode(columnHeadings[i]);
				
				assert(x >= 0 and x < kAACodeCount);
				assert(y >= 0 and y < kAACodeCount);
				
				mTable[x][y] = score;
				
				if (score > mHighestScore)
					mHighestScore = score;
				
				if (score < mLowestScore)
					mLowestScore = score;
			}
		}

		++lineNumber;
	}
	
	for (int x = 0; x < kAACodeCount; ++x)
	{
		for (int y = 0; y < kAACodeCount; ++y)
		{
			if (mTable[x][y] == kSentinalScore)
			{
				if (x == y)
					mTable[x][y] = 1;
				else
					mTable[x][y] = mLowestScore;
			}
		}
	}
	
	for (int x = 0; x < kAACodeCount; ++x)
	{
		mTable[x][kSentinalCode] = kSentinalScore;
		mTable[kSentinalCode][x] = kSentinalScore;
	}
}

int32 CMatrix::Score(const CSequence& inSeqA, const CSequence& inSeqB,
	int32 inOpenCost, int32 inExtendCost) const
{
	int32 result = 0;
	
	CSequence::const_iterator ai = inSeqA.begin();
	CSequence::const_iterator bi = inSeqB.begin();

//	// strip off leading 'gaps'
//
//	if (*ai == '-')
//	{
//		do
//		{
//			++ai;
//			++bi;
//		}		
//		while (ai != inSeqA.end() and bi != inSeqB.end() and *ai == '-');
//	}
//	else if (*bi == '-')
//	{
//		do
//		{
//			++ai;
//			++bi;
//		}		
//		while (ai != inSeqA.end() and bi != inSeqB.end() and *bi == '-');
//	}

	CSequence::const_iterator ae = inSeqA.end();
	CSequence::const_iterator be = inSeqB.end();

//	// strip off trailing 'gaps'
//
//	if (ae != ai and *(ae - 1) == '-')
//	{
//		do
//		{
//			--ae;
//			--be;
//		}		
//		while (ae != ai and be != bi and *(ae - 1) == '-');
//	}
//	else if (be != bi and *(be - 1) == '-')
//	{
//		do
//		{
//			--ae;
//			--be;
//		}		
//		while (ae != ai and be != bi and *(be - 1) == '-');
//	}
	
	while (ai != ae and bi != be)
	{
		if (*ai == '-')
		{
			result -= inOpenCost;
			
			++ai;
			++bi;
			
			while (ai != ae and bi != be and *ai == '-')
			{
				result -= inExtendCost;

				++ai;
				++bi;
			}
		}
		else if (*bi == '-')
		{
			result -= inOpenCost;
			
			++ai;
			++bi;
			
			while (ai != ae and bi != be and *bi == '-')
			{
				result -= inExtendCost;

				++ai;
				++bi;
			}
		}
		else
		{
			result += Score(*ai, *bi);
		
			++ai;
			++bi;
		}
	}
	
	return result;
}

CPSSMatrix::CPSSMatrix(const CSequence& inQuery, const CMatrix& inMatrix)
{
	mScoreCount = inQuery.length();
	mScores = new PSScore[mScoreCount];
	
	PSScore* s = mScores;
	
	for (CSequence::const_iterator aa = inQuery.begin(); aa != inQuery.end(); ++aa, ++s)
	{
		for (unsigned char t = 0; t < kAACodeCount; ++t)
			(*s)[t] = inMatrix.Score(*aa, t);
	}
}

CPSSMatrix::~CPSSMatrix()
{
	delete[] mScores;
}
