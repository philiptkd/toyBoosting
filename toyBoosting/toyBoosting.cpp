// toyBoosting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <algorithm>

using namespace std;

struct dataPoint
{
	double x;
	double y;
	double p;
};

class Booster
{
public:
	int m;						//number of training examples
	int T;						//number of weak hypotheses to find
	int numDims;				//number of dimensions in toy data
	vector<vector<double> > x;	//the training data
	vector<int> y;				//the labels for training data
	vector<double> thetas;		//the weights for each of the weak hypotheses
	vector<int> dims;			//the dimensions used for the weak hypotheses (stumps)
	vector<double> threshs;		//the thresholds for the weak hypotheses (stumps)
	vector<int> directions;		//the directions of the weak hypothesis stumps
	vector<double> w;			//the weights for each of the terms in the cost function
	vector<double> p;			//the weights for the training examples, used to form weak hypotheses

	vector<dataPoint> flatData;	//x, y, and p data with only 1 dimension of x

	Booster(int, int, int);
	void boost();
	vector<double> getStump();
	double sumThetaPhi(int xCol);
	int phiOfX(int xCol, int dim, double thresh, int direction);
};

int main()
{
	Booster B = Booster(100, 20, 2);
	B.boost();

    return 0;
}

Booster::Booster(int _m, int _T, int _numDims) {
	//save input parameters
	m = _m;
	T = _T;
	numDims = _numDims;

	srand(time(NULL));		//seed rand()

	//fill inputs x with random data from 0 to 1
	x = vector<vector<double> >(numDims, vector<double>(m));
	for (int i = 0; i < numDims; i++) {
		for (int j = 0; j < m; j++) {
			x[i][j] = rand()/(double)RAND_MAX;
		}
	}

	//set labels equal to 1 only if the inputs are less than arbThresh in all dimensions
	double arbThresh = 0.6;
	y = vector<int>(m, -1);
	for (int i = 0; i < m; i++) {
		bool genuine = true;
		for (int j = 0; j < numDims; j++) {
			if (x[j][i] >= arbThresh) {
				genuine = false;
				break;
			}
		}
		if (genuine) {
			y[i] = 1;
		}
	}
	
	//initialize these to 0
	thetas = vector<double>(T, 0);
	dims = vector<int>(T, 0);
	threshs = vector<double>(T, 0);
	directions = vector<int>(T, 0);
	w = vector<double>(m, 0);

	//give flatData a size
	flatData = vector<dataPoint>(m);

	//initialize p to 1/m
	p = vector<double>(m, 1/(double)m);
}

void Booster::boost() {
	//do T times
	for (int t = 0; t < T; t++) {
		//construct weak hypothesis phi(t) using p(i)
		vector<double> hypothesis = getStump();
		dims[t] = hypothesis[0];
		threshs[t] = hypothesis[1];
		directions[t] = hypothesis[2];

		//update w
		for (int i = 0; i < m; i++) {
			w[i] = exp(-y[i] * sumThetaPhi(i));
		}

		//compute weight theta(t)
		double Wplus = 0;
		double Wminus = 0;
		for (int i = 0; i < m; i++) {	//for each training example
			if (y[i] * phiOfX(i, dims[t], threshs[t], directions[t]) == 1) {	//if this hypothesis is correct
				Wplus = Wplus + w[i];	//add
			}
			else {	//it got it incorrect
				Wminus = Wminus + w[i];
			}
		}
		thetas[t] = log(Wplus / Wminus) / 2.0;

		//update weights/distribution p(i)
		double wSum = Wplus + Wminus;
		for (int i = 0; i < m; i++) {
			p[i] = w[i] / wSum;
		}

		//count number wrong
		int numWrong = 0;
		for (int i = 0; i < m; i++) {
			if (y[i] * sumThetaPhi(i) < 0) {
				numWrong++;
			}
		}

		//print error after t iterations
		cout << "iteration " << t << ": " << wSum / m << "\n";
		cout << "# wrong: " << numWrong << "\n\n";
	}

	int n;
	cin >> n;
}

bool sortFunc(dataPoint& first, dataPoint& second)
{
	return first.x < second.x;
}

vector<double> Booster::getStump() {
	double bestErr = 10 * m;		//arbitrary high error
	int bestErrDim;				//dimension of best stump
	double bestErrThresh;		//threshold of best stump
	double bestErrDirection;	//direction of best stump. 0 means less than threshold is better

	for (int j = 0; j < numDims; j++) {		//for each dimension j of the data, x
		//build flatData and sort it by x[j]
		for (int i = 0; i < m; i++)
		{
			flatData[i].x = x[j][i];
			flatData[i].y = y[i];
			flatData[i].p = p[i];
		}
		sort(flatData.begin(), flatData.end(), sortFunc);

		for (int i = 0; i < m; i++) {		//for each possible threshold
			double sumErr = 0;
			for (int k = 0; k < m; k++) {		//for each training example
				//if k <= i, then phi=(s-x_k) is positive.
				int phiSign = (k <= i) ? 1 : -1;

				//we need only check the sign of y[k]
				sumErr = (flatData[k].y*phiSign < 0) ? (sumErr + flatData[k].p) : sumErr;
			}
			if (sumErr < bestErr) {
				bestErr = sumErr;
				bestErrDim = j;
				bestErrThresh = flatData[i].x;
				bestErrDirection = 0;
			}
			else if (1 - sumErr < bestErr) {
				bestErr = 1 - sumErr;
				bestErrDim = j;
				bestErrThresh = flatData[i].x;
				bestErrDirection = 1;
			}
		}
	}
	
	vector<double> ret;
	ret.push_back(bestErrDim);
	ret.push_back(bestErrThresh);
	ret.push_back(bestErrDirection);
	return ret;
}

double Booster::sumThetaPhi(int xCol) {
	double retSum = 0;
	for (int j = 0; j < T; j++) {
		int phi = phiOfX(xCol, dims[j], threshs[j], directions[j]);
		retSum = retSum + thetas[j] * phi;
	}
	return retSum;
}

int Booster::phiOfX(int xCol, int dim, double thresh, int direction) {
	if (direction == 0) {
		return (x[dim][xCol] <= thresh) ? 1 : -1;
	}
	else {
		return (x[dim][xCol] >= thresh) ? 1 : -1;
	}
}