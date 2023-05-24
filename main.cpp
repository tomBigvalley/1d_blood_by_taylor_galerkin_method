#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <math.h>
#include <cmath>
#include "lib/eigen/Eigen/Core"
#include "lib/eigen/Eigen/LU"
#include <cstdio>
#include <iomanip>
#include "shapefunction.h"
#include "gauss.h"
#include "params.h"
#include "left-part.h"
#include "exception-args.h"
#include "exception-manager.h"

using namespace std;
using namespace Eigen;

void arrest()
{
  fstream ifs1("input/element_d.dat");
  for (int i = 0; i < ELEMENT_NUM; i++)
  {
    vector<double> element_t(2, 0);
    for (int j = 0; j < 2; j++)
    {
      ifs1 >> element_t[j];
    }
    element[i] = element_t;
  }
  ifs1.close();

  string str;
  ifstream ifs2("input/node_d.dat");
  while (getline(ifs2, str))
  {
    istringstream ss(str);
    string tmp;
    vector<double> tmp_x;
    for (int j = 0; j < 1; j++)
    {
      getline(ss, tmp, ' ');
      tmp_x.push_back(stod(tmp));
    }
    x.push_back(tmp_x[0]);
  }
  ifs2.close();
}

// init Q and A (which we want to calculate)
void initVariables()
{
  for (int i = 0; i < NODE_NUM; i++)
  {
    // 初期状態のnodeに与える速度はノード点の位置によって変える
    if (i < 60)
    {
      area[0][i] = A0;
      velocity[0][i] = v0;
      flowQuantity[0][i] = area[0][i] * velocity[0][i];
    }
    else
    {
      area[0][i] = A0 / 1.5e0;
      velocity[0][i] = v0 * 1.5e0;
      flowQuantity[0][i] = area[0][i] * velocity[0][i];
    }
  }
}

void output()
{
  int iteratedTime;

  fstream ifsIteratedTime("output/dat/iteratedTime.dat");
  ifsIteratedTime >> iteratedTime;
  ifsIteratedTime.close();

  ofstream ofs("output/dat/flowQuantity.dat");
  for (int i = 0; i < 12; i++)
  {
    for (int j = 0; j < NODE_NUM; j++)
    {
      ofs << flowQuantity[i * 50][j] << " ";
    }
    ofs << endl;
  }
  ofs.close();

  ofstream ofs2("output/dat/area.dat");
  for (int i = 0; i < 12; i++)
  {
    for (int j = 0; j < NODE_NUM; j++)
    {
      ofs2 << area[i * 50][j] << " ";
    }
    ofs2 << endl;
  }
  ofs2.close();
}

void exec()
{
  LeftPart leftPart = LeftPart::newLeftPart();
  // 右辺の1~5項の内、計算をskipするものを指定する
  vector<int> indices = {2, 4};
  LeftPart::disable(leftPart, indices);

  ShapeFunction1D shape;

  // 時間ステップごとに計算
  for (int i = 0; i < M; i++)
  {
    MatrixXd A_area = MatrixXd::Zero(NODE_NUM, NODE_NUM);
    VectorXd b_area = VectorXd::Zero(NODE_NUM);
    MatrixXd A_flowQuantity = MatrixXd::Zero(NODE_NUM, NODE_NUM);
    VectorXd b_flowQuantity = VectorXd::Zero(NODE_NUM);

    for (int j = 0; j < ELEMENT_NUM; j++)
    {
      int ele0 = element[j][0];
      int ele1 = element[j][1];

      for (int k = 0; k < 2; k++)
      {
        Gauss g(1);
        vector<double> N(2, 0e0);
        vector<double> dNdr(2, 0e0);

        shape.P2_N(N, g.point[k]);
        shape.P2_dNdr(dNdr, g.point[k]);

        double dxdr = dNdr.at(0) * x.at(ele0) + dNdr.at(1) * x.at(ele1);

        A_area(ele0, ele0) += N.at(0) * N.at(0) * g.weight[k] * dxdr;
        A_area(ele0, ele1) += N.at(0) * N.at(1) * g.weight[k] * dxdr;
        A_area(ele1, ele0) += N.at(1) * N.at(0) * g.weight[k] * dxdr;
        A_area(ele1, ele1) += N.at(1) * N.at(1) * g.weight[k] * dxdr;

        A_flowQuantity(ele0, ele0) += N.at(0) * N.at(0) * g.weight[k] * dxdr;
        A_flowQuantity(ele0, ele1) += N.at(0) * N.at(1) * g.weight[k] * dxdr;
        A_flowQuantity(ele1, ele0) += N.at(1) * N.at(0) * g.weight[k] * dxdr;
        A_flowQuantity(ele1, ele1) += N.at(1) * N.at(1) * g.weight[k] * dxdr;
      }
    }

    // B.C. : 0番目のnode点では常に流路面積, 速度一定
    area[i][0] = A0;
    flowQuantity[i][0] = A0 * v0;

    for (int j = 0; j < ELEMENT_NUM; j++)
    {
      int ele0 = element[j][0];
      int ele1 = element[j][1];
      double flow0 = flowQuantity[i][ele0], flow1 = flowQuantity[i][ele1];
      double area0 = area[i][ele0], area1 = area[i][ele1];

      for (int k = 0; k < 2; k++)
      {
        Gauss g(1);
        vector<double> N(2, 0e0);
        vector<double> dNdr(2, 0e0);
        vector<double> dNdx(2, 0e0);
        vector<double> dNinvdr(2, 0e0);
        vector<double> dNinvdx(2, 0e0);

        shape.P2_N(N, g.point[k]);
        shape.P2_dNdr(dNdr, g.point[k]);
        shape.P2_dNinvdr(dNinvdr, g.point[k]);

        double dxdr = dNdr.at(0) * x.at(ele0) + dNdr.at(1) * x.at(ele1);
        double drdx = 1e0 / dxdr;

        dNdx.at(0) = dNdr.at(0) * drdx;
        dNdx.at(1) = dNdr.at(1) * drdx;

        dNinvdx.at(0) = dNinvdr.at(0) * drdx;
        dNinvdx.at(1) = dNinvdr.at(1) * drdx;

        double Q = N.at(0) * flow0 + N.at(1) * flow1;
        double A = N.at(0) * area0 + N.at(1) * area1;
        double dQdx = dNdx.at(0) * flow0 + dNdx.at(1) * flow1;
        double dAdx = dNdx.at(0) * area0 + dNdx.at(1) * area1;
        double dAinvdx = dNinvdx.at(0) / area0 + dNinvdx.at(1) / area1;
        double dQQAdx = dAinvdx * Q * Q + (1 / A) * dQdx * Q + (1 / A) * Q * dQdx;

        if (leftPart.firstTerm.shouldCalculate)
        {
          b_area(ele0) += N.at(0) * A * g.weight[k] * dxdr;
          b_area(ele1) += N.at(1) * A * g.weight[k] * dxdr;
          b_flowQuantity(ele0) += N.at(0) * Q * g.weight[k] * dxdr;
          b_flowQuantity(ele1) += N.at(1) * Q * g.weight[k] * dxdr;

          if (b_area(ele0) < 0e0 || b_area(ele1) < 0e0 || b_flowQuantity(ele0) < 0e0 || b_flowQuantity(ele1) < 0e0 || Q < 0e0 || A < 0e0)
          {
            ExceptionArgs args = ExceptionArgs::newCheckArgs(b_area(ele0), b_area(ele1), b_flowQuantity(ele0), b_flowQuantity(ele1), i, j, leftPart.firstTerm.index);
            ExceptionManager::check(args);
          }
        }
        if (leftPart.secondTerm.shouldCalculate)
        {
          b_area(ele0) += dNdx.at(0) * dt * (Q + dt / 2e0 * (-K_R * Q / A)) * g.weight[k] * dxdr;
          b_area(ele1) += dNdx.at(1) * dt * (Q + dt / 2e0 * (-K_R * Q / A)) * g.weight[k] * dxdr;
          b_flowQuantity(ele0) += dNdx.at(0) * dt * (Q * Q / A + betha / 3e0 / rho * pow(A, 1.5e0) + dt * Q / A * (-K_R * Q / A)) * g.weight[k] * dxdr;
          b_flowQuantity(ele1) += dNdx.at(1) * dt * (Q * Q / A + betha / 3e0 / rho * pow(A, 1.5e0) + dt * Q / A * (-K_R * Q / A)) * g.weight[k] * dxdr;

          if (b_area(ele0) < 0e0 || b_area(ele1) < 0e0 || b_flowQuantity(ele0) < 0e0 || b_flowQuantity(ele1) < 0e0 || Q < 0e0 || A < 0e0)
          {
            ExceptionArgs args = ExceptionArgs::newCheckArgs(b_area(ele0), b_area(ele1), b_flowQuantity(ele0), b_flowQuantity(ele1), i, j, leftPart.secondTerm.index);
            ExceptionManager::check(args);
          }
        }
        if (leftPart.thirdTerm.shouldCalculate)
        { // b_area has no third term
          b_flowQuantity(ele0) += -N.at(0) * dt * dt / 2.0e0 * (K_R * Q / (A * A) * dQdx + (K_R / A) * dQQAdx + (betha * K_R / (2e0 * rho)) * pow(A, -5e-1) * dAdx) * g.weight[k] * dxdr;
          b_flowQuantity(ele1) += -N.at(1) * dt * dt / 2.0e0 * (K_R * Q / (A * A) * dQdx + (K_R / A) * dQQAdx + (betha * K_R / (2e0 * rho)) * pow(A, -5e-1) * dAdx) * g.weight[k] * dxdr;

          if (b_area(ele0) < 0e0 || b_area(ele1) < 0e0 || b_flowQuantity(ele0) < 0e0 || b_flowQuantity(ele1) < 0e0 || Q < 0e0 || A < 0e0)
          {
            ExceptionArgs args = ExceptionArgs::newCheckArgs(b_area(ele0), b_area(ele1), b_flowQuantity(ele0), b_flowQuantity(ele1), i, j, leftPart.thirdTerm.index);
            ExceptionManager::check(args);
          }
        }
        if (leftPart.fourthTerm.shouldCalculate)
        {
          b_area(ele0) += -dNdx.at(0) * dt * dt / 2.0e0 * (dQQAdx + (betha / (2e0 * rho)) * pow(A, -5e-1) * dAdx) * g.weight[k] * dxdr;
          b_area(ele1) += -dNdx.at(1) * dt * dt / 2.0e0 * (dQQAdx + (betha / (2e0 * rho)) * pow(A, -5e-1) * dAdx) * g.weight[k] * dxdr;
          b_flowQuantity(ele0) += -dNdx.at(0) * dt * dt / 2.0e0 * (-(Q * Q / A) * dQdx + (betha / 2e0 / rho) * pow(A, 5e-1) * dQdx + 2e0 * Q / A * dQQAdx + betha / rho * pow(A, -5e-1) * Q * dAdx) * g.weight[k] * dxdr;
          b_flowQuantity(ele1) += -dNdx.at(1) * dt * dt / 2.0e0 * (-(Q * Q / A) * dQdx + (betha / 2e0 / rho) * pow(A, 5e-1) * dQdx + 2e0 * Q / A * dQQAdx + betha / rho * pow(A, -5e-1) * Q * dAdx) * g.weight[k] * dxdr;

          if (b_area(ele0) < 0e0 || b_area(ele1) < 0e0 || b_flowQuantity(ele0) < 0e0 || b_flowQuantity(ele1) < 0e0 || Q < 0e0 || A < 0e0)
          {
            ExceptionArgs args = ExceptionArgs::newCheckArgs(b_area(ele0), b_area(ele1), b_flowQuantity(ele0), b_flowQuantity(ele1), i, j, leftPart.fourthTerm.index);
            ExceptionManager::check(args);
          }
        }
        if (leftPart.fifthTerm.shouldCalculate)
        { // b_area has no fifth term
          b_flowQuantity(ele0) += N.at(0) * dt * (-K_R * Q / A - dt / 2e0 * K_R * K_R * Q / A / A) * g.weight[k] * dxdr;
          b_flowQuantity(ele1) += N.at(1) * dt * (-K_R * Q / A - dt / 2e0 * K_R * K_R * Q / A / A) * g.weight[k] * dxdr;

          if (b_area(ele0) < 0e0 || b_area(ele1) < 0e0 || b_flowQuantity(ele0) < 0e0 || b_flowQuantity(ele1) < 0e0 || Q < 0e0 || A < 0e0)
          {
            ExceptionArgs args = ExceptionArgs::newCheckArgs(b_area(ele0), b_area(ele1), b_flowQuantity(ele0), b_flowQuantity(ele1), i, j, leftPart.fifthTerm.index);
            ExceptionManager::check(args);
          }
        }
      }
    }

    Eigen::VectorXd x_area = A_area.fullPivLu().solve(b_area);
    Eigen::VectorXd x_flowQuantity = A_flowQuantity.fullPivLu().solve(b_flowQuantity);

    for (int j = 0; j < NODE_NUM; j++)
    {
      area[i + 1][j] = x_area(j);
      flowQuantity[i + 1][j] = x_flowQuantity(j);
    }
  }
}

int main()
{
  arrest();

  initVariables();

  exec();

  output();
}