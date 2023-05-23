#include <math.h>
#include <cmath>

int ELEMENT_NUM = 100;
int NODE_NUM = ELEMENT_NUM + 1;

const double L = 1.0e0;                                      // tubeの長さ[m]
const int M = 1200;                                          // 時間ステップ数[-]
const double dt = 1.0e-03;                                   // 時間刻み[s]
const double DELTA_X = L / ELEMENT_NUM;                      // 要素の長さ[m]
const double PI = M_PI;                                      // 円周率
const double r0 = 1.0e-03;                                   // 初期状態のtubeの半径[m]
const double h0 = 1.0e-04;                                   // 初期状態のtubeの厚さ[m]
const double A0 = PI * pow(r0 - h0, 2.0e0);                  // 初期状態のtubeの流路面積（位置座標によらない）[m^2]
const double K_R = 3.3e-06;                                  // 粘性抵抗K_R[m^2/s]
const double rho = 1060000.0;                                // 密度[g/m^3]
const double E = 1.0e08;                                     // ヤング率(0.1MPa)[g/m/s^2]
const double betha = 4.0e0 / 3.0e0 * sqrt(PI) * h0 * E / A0; // ß
// const double dbetha_dx = -4.0 / 3.0 * h0 * sqrt(PI) / PI / PI * E * 1.0e8; // dß/dx
double v0 = 1.0e0; // 所定位置における初期状態のtubeの流速[m/s]

// [時刻][node番号]
vector<vector<double>> area(M + 1, vector<double>(NODE_NUM, 0e0));
vector<vector<double>> velocity(M + 1, vector<double>(NODE_NUM, 0e0));
vector<vector<double>>
    flowQuantity(M + 1, vector<double>(NODE_NUM, 0e0));