#include "FWCore/Framework/interface/Event.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"

#include "babymaker/bmaker/interface/weight_tools.hh"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

TString outname_wt;

weight_tools::weight_tools(TString outname){
  theoryWeights.clear();

  outname_wt = outname;
  bad_pu17_datasets = {"TTJets_SingleLeptFromT_TuneCP5",
                       "DYJetsToLL_M-50_HT-1200to2500_TuneCP5",
                       "QCD_HT300to500_TuneCP5",
                       "QCD_HT700to1000_TuneCP5",
                       "ZJetsToNuNu_HT-100To200",
                       "ZJetsToNuNu_HT-200To400",
                       "ZJetsToNuNu_HT-800To1200",
                       "ZJetsToNuNu_HT-2500ToInf",
                       "TTZToLLNuNu_M-10_TuneCP5",
                       "TTWJetsToLNu_TuneCP5",
                       "ZZ_TuneCP5"};
  bad_pu17 = false;

  // obtained with scripts/get_puweights.py
  if(outname_wt.Contains("RunIISummer16")) {
    w_pu_down  = vector<double>({3.510e-01, 1.057e+00, 1.197e+00, 1.063e+00, 1.231e+00, 1.306e+00, 9.260e-01, 
      7.621e-01, 1.089e+00, 1.333e+00, 1.486e+00, 1.532e+00, 1.496e+00, 1.501e+00, 1.498e+00, 1.443e+00, 
      1.366e+00, 1.296e+00, 1.224e+00, 1.163e+00, 1.122e+00, 1.094e+00, 1.065e+00, 1.039e+00, 1.019e+00, 
      1.008e+00, 9.968e-01, 9.833e-01, 9.749e-01, 9.592e-01, 9.145e-01, 8.753e-01, 8.075e-01, 7.335e-01, 
      6.515e-01, 5.645e-01, 4.674e-01, 3.758e-01, 2.889e-01, 2.153e-01, 1.504e-01, 1.005e-01, 6.458e-02, 
      3.955e-02, 2.399e-02, 1.386e-02, 7.572e-03, 4.198e-03, 2.220e-03, 1.189e-03, 6.461e-04, 3.844e-04, 
      2.714e-04, 2.435e-04, 2.917e-04, 4.006e-04, 5.506e-04, 7.688e-04, 1.132e-03, 1.466e-03, 2.504e-03, 
      3.381e-03, 3.440e-03, 4.134e-03, 4.689e-03, 3.913e-03, 3.534e-03, 2.909e-03, 2.829e-03, 2.439e-03, 
      1.932e-03, 1.705e-03, 1.486e-03, 1.236e-03, 8.924e-04});
  
    w_pu_nom  = vector<double>({3.388e-01, 8.277e-01, 1.139e+00, 9.310e-01, 1.105e+00, 1.187e+00, 8.008e-01, 
      4.921e-01, 7.396e-01, 8.757e-01, 9.640e-01, 1.075e+00, 1.124e+00, 1.176e+00, 1.203e+00, 1.207e+00, 
      1.199e+00, 1.180e+00, 1.141e+00, 1.094e+00, 1.062e+00, 1.053e+00, 1.052e+00, 1.049e+00, 1.049e+00, 
      1.060e+00, 1.072e+00, 1.081e+00, 1.098e+00, 1.111e+00, 1.094e+00, 1.086e+00, 1.042e+00, 9.848e-01, 
      9.114e-01, 8.254e-01, 7.184e-01, 6.120e-01, 5.038e-01, 4.064e-01, 3.105e-01, 2.293e-01, 1.643e-01, 
      1.131e-01, 7.764e-02, 5.108e-02, 3.192e-02, 2.030e-02, 1.229e-02, 7.443e-03, 4.404e-03, 2.610e-03, 
      1.558e-03, 9.718e-04, 7.345e-04, 6.810e-04, 7.393e-04, 9.318e-04, 1.332e-03, 1.731e-03, 3.006e-03, 
      4.147e-03, 4.321e-03, 5.327e-03, 6.203e-03, 5.317e-03, 4.938e-03, 4.182e-03, 4.186e-03, 3.719e-03, 
      3.035e-03, 2.763e-03, 2.486e-03, 2.135e-03, 1.594e-03});
  
    w_pu_up  = vector<double>({3.301e-01, 6.518e-01, 1.078e+00, 8.173e-01, 1.005e+00, 1.069e+00, 7.303e-01, 
      3.452e-01, 4.988e-01, 6.007e-01, 6.320e-01, 7.344e-01, 8.268e-01, 9.128e-01, 9.610e-01, 9.885e-01, 
      1.024e+00, 1.051e+00, 1.048e+00, 1.025e+00, 1.002e+00, 9.996e-01, 1.015e+00, 1.036e+00, 1.057e+00, 
      1.087e+00, 1.120e+00, 1.153e+00, 1.195e+00, 1.235e+00, 1.246e+00, 1.272e+00, 1.260e+00, 1.233e+00, 
      1.182e+00, 1.111e+00, 1.005e+00, 8.924e-01, 7.702e-01, 6.564e-01, 5.348e-01, 4.251e-01, 3.310e-01, 
      2.497e-01, 1.894e-01, 1.386e-01, 9.691e-02, 6.935e-02, 4.744e-02, 3.256e-02, 2.182e-02, 1.454e-02, 
      9.534e-03, 6.171e-03, 4.309e-03, 3.109e-03, 2.269e-03, 1.888e-03, 1.988e-03, 2.191e-03, 3.561e-03, 
      4.837e-03, 5.075e-03, 6.358e-03, 7.554e-03, 6.620e-03, 6.292e-03, 5.458e-03, 5.599e-03, 5.100e-03, 
      4.272e-03, 3.993e-03, 3.691e-03, 3.258e-03, 2.501e-03});
  }

  // 2017 weight
  else if(outname_wt.Contains("RunIIFall17")) {
    for(size_t i = 0; i < bad_pu17_datasets.size(); i++) 
    	if(outname_wt.Contains(bad_pu17_datasets.at(i))) bad_pu17 = true;
    if(bad_pu17) {	
	  w_pu_down  = vector<double>({2.165e-04, 6.838e-01, 4.274e-01, 1.983e-01, 2.317e-01, 3.736e-01, 4.795e-01,
	    5.454e-01, 3.627e-01, 6.375e-01, 6.994e-01, 6.783e-01, 6.048e-01, 5.205e-01, 5.434e-01, 1.598e+00, 
		1.222e+00, 6.668e-01, 1.551e+00, 1.500e+00, 1.564e+00, 1.490e+00, 1.586e+00, 1.508e+00, 1.535e+00, 
		1.557e+00, 1.481e+00, 1.371e+00, 1.445e+00, 1.392e+00, 1.344e+00, 1.240e+00, 1.126e+00, 1.036e+00, 
		9.828e-01, 9.052e-01, 8.634e-01, 8.075e-01, 7.203e-01, 8.051e-01, 8.622e-01, 8.988e-01, 9.887e-01, 
		1.184e+00, 1.320e+00, 1.483e+00, 1.781e+00, 1.575e+00, 1.459e+00, 1.183e+00, 9.838e-01, 7.386e-01, 
		5.159e-01, 3.667e-01, 2.485e-01, 1.701e-01, 1.074e-01, 6.519e-02, 4.704e-02, 2.938e-02, 2.020e-02, 
		1.299e-02, 9.617e-03, 7.178e-03, 6.009e-03, 4.240e-03, 3.483e-03, 3.461e-03, 2.880e-03, 2.896e-03, 
		1.988e-03, 2.175e-03, 1.807e-03, 1.880e-03, 1.850e-03});

      w_pu_nom  = vector<double>({2.070e-04, 6.181e-01, 3.277e-01, 2.051e-01, 2.010e-01, 3.351e-01, 4.597e-01, 
	    4.869e-01, 2.512e-01, 4.290e-01, 4.791e-01, 4.882e-01, 4.627e-01, 3.959e-01, 4.078e-01, 1.185e+00,
		9.225e-01, 5.255e-01, 1.270e+00, 1.268e+00, 1.362e+00, 1.328e+00, 1.437e+00, 1.376e+00, 1.404e+00,
		1.434e+00, 1.384e+00, 1.306e+00, 1.408e+00, 1.386e+00, 1.367e+00, 1.289e+00, 1.198e+00, 1.125e+00,
		1.085e+00, 1.009e+00, 9.671e-01, 9.021e-01, 7.927e-01, 8.585e-01, 8.760e-01, 8.626e-01, 9.005e-01,
		1.043e+00, 1.157e+00, 1.338e+00, 1.704e+00, 1.639e+00, 1.685e+00, 1.535e+00, 1.446e+00, 1.232e+00,
		9.753e-01, 7.815e-01, 5.921e-01, 4.488e-01, 3.104e-01, 2.042e-01, 1.580e-01, 1.047e-01, 7.561e-02,
		5.057e-02, 3.854e-02, 2.930e-02, 2.473e-02, 1.742e-02, 1.419e-02, 1.391e-02, 1.141e-02, 1.133e-02,
		7.722e-03, 8.436e-03, 7.045e-03, 7.424e-03, 7.444e-03});
    	
	  w_pu_up  = vector<double>({2.006e-04, 5.340e-01, 2.684e-01, 2.062e-01, 1.732e-01, 2.969e-01, 4.441e-01, 
	    4.456e-01, 2.023e-01, 2.784e-01, 3.417e-01, 3.400e-01, 3.514e-01, 3.066e-01, 3.129e-01, 8.969e-01, 
		6.938e-01, 4.049e-01, 1.019e+00, 1.052e+00, 1.162e+00, 1.165e+00, 1.287e+00, 1.251e+00, 1.286e+00,
		1.317e+00, 1.277e+00, 1.220e+00, 1.337e+00, 1.343e+00, 1.352e+00, 1.299e+00, 1.230e+00, 1.180e+00,
		1.162e+00, 1.100e+00, 1.067e+00, 1.003e+00, 8.837e-01, 9.526e-01, 9.563e-01, 9.125e-01, 9.102e-01,
		1.000e+00, 1.058e+00, 1.186e+00, 1.501e+00, 1.477e+00, 1.595e+00, 1.563e+00, 1.612e+00, 1.524e+00,
		1.349e+00, 1.213e+00, 1.031e+00, 8.743e-01, 6.723e-01, 4.880e-01, 4.128e-01, 2.962e-01, 2.295e-01,
		1.632e-01, 1.310e-01, 1.040e-01, 9.090e-02, 6.571e-02, 5.441e-02, 5.374e-02, 4.405e-02, 4.342e-02,
		2.923e-02, 3.151e-02, 2.600e-02, 2.716e-02, 2.715e-02});
	}
    else{
      w_pu_down  = vector<double>({1.561e-01, 0.000e+00, 3.027e+00, 1.404e+00, 2.696e+00, 1.890e+00, 2.587e+00, 
        1.766e+00, 9.632e-01, 2.078e+00, 2.628e+00, 2.069e+00, 1.549e+00, 1.537e+00, 1.452e+00, 1.459e+00, 
        1.498e+00, 1.423e+00, 1.344e+00, 1.377e+00, 1.321e+00, 1.364e+00, 1.354e+00, 1.360e+00, 1.436e+00,
        1.349e+00, 1.332e+00, 1.345e+00, 1.282e+00, 1.274e+00, 1.213e+00, 1.119e+00, 1.069e+00, 1.026e+00,
        8.585e-01, 8.533e-01, 7.821e-01, 7.792e-01, 7.155e-01, 6.973e-01, 7.114e-01, 8.063e-01, 9.559e-01,
        1.089e+00, 1.278e+00, 1.415e+00, 1.550e+00, 1.444e+00, 1.261e+00, 1.083e+00, 8.727e-01, 7.160e-01,
        5.104e-01, 3.571e-01, 2.403e-01, 1.503e-01, 1.048e-01, 6.315e-02, 4.187e-02, 2.714e-02, 1.787e-02,
        1.304e-02, 9.189e-03, 8.089e-03, 5.361e-03, 4.121e-03, 3.098e-03, 2.749e-03, 2.706e-03, 2.862e-03,
        1.490e-03, 2.780e-03, 2.171e-03, 1.902e-03, 1.872e-03});

      w_pu_nom  = vector<double>({1.492e-01, 0.000e+00, 2.321e+00, 1.453e+00, 2.339e+00, 1.695e+00, 2.480e+00, 
        1.576e+00, 6.670e-01, 1.398e+00, 1.800e+00, 1.490e+00, 1.185e+00, 1.169e+00, 1.090e+00, 1.082e+00, 
        1.131e+00, 1.121e+00, 1.101e+00, 1.164e+00, 1.150e+00, 1.216e+00, 1.227e+00, 1.241e+00, 1.314e+00, 
        1.243e+00, 1.245e+00, 1.282e+00, 1.249e+00, 1.269e+00, 1.234e+00, 1.163e+00, 1.137e+00, 1.115e+00,
        9.477e-01, 9.515e-01, 8.761e-01, 8.705e-01, 7.875e-01, 7.435e-01, 7.228e-01, 7.738e-01, 8.706e-01,
        9.593e-01, 1.121e+00, 1.277e+00, 1.483e+00, 1.503e+00, 1.456e+00, 1.405e+00, 1.282e+00, 1.194e+00,
        9.649e-01, 7.610e-01, 5.726e-01, 3.967e-01, 3.029e-01, 1.978e-01, 1.406e-01, 9.670e-02, 6.690e-02,
        5.077e-02, 3.683e-02, 3.302e-02, 2.206e-02, 1.694e-02, 1.262e-02, 1.105e-02, 1.072e-02, 1.120e-02,
        5.787e-03, 1.078e-02, 8.464e-03, 7.511e-03, 7.531e-03});

      w_pu_up  = vector<double>({1.446e-01, 0.000e+00, 1.900e+00, 1.460e+00, 2.015e+00, 1.502e+00, 2.396e+00, 
        1.443e+00, 5.374e-01, 9.075e-01, 1.284e+00, 1.037e+00, 9.000e-01, 9.055e-01, 8.363e-01, 8.189e-01, 
        8.505e-01, 8.637e-01, 8.827e-01, 9.656e-01, 9.817e-01, 1.066e+00, 1.100e+00, 1.129e+00, 1.203e+00, 
        1.141e+00, 1.149e+00, 1.197e+00, 1.186e+00, 1.229e+00, 1.219e+00, 1.172e+00, 1.169e+00, 1.169e+00,
        1.015e+00, 1.037e+00, 9.668e-01, 9.678e-01, 8.778e-01, 8.250e-01, 7.890e-01, 8.186e-01, 8.800e-01,
        9.202e-01, 1.025e+00, 1.132e+00, 1.306e+00, 1.354e+00, 1.378e+00, 1.431e+00, 1.430e+00, 1.477e+00,
        1.334e+00, 1.181e+00, 9.974e-01, 7.728e-01, 6.562e-01, 4.728e-01, 3.674e-01, 2.737e-01, 2.031e-01,
        1.638e-01, 1.252e-01, 1.172e-01, 8.109e-02, 6.387e-02, 4.840e-02, 4.269e-02, 4.138e-02, 4.290e-02,
        2.191e-02, 4.027e-02, 3.124e-02, 2.748e-02, 2.746e-02});
	}
  }
  else{
    w_pu_down = vector<double>({1.0});
    w_pu_nom  = vector<double>({1.0});
    w_pu_up   = vector<double>({1.0});
  }
}
weight_tools::~weight_tools() {}

float weight_tools::pileupWeight(unsigned int ntrupv_mean, int type)
{
  if (ntrupv_mean>=w_pu_down.size()) ntrupv_mean = w_pu_down.size()-1;

  if(type==-1) return static_cast<float>(w_pu_down.at(ntrupv_mean));
  else if(type==1) return static_cast<float>(w_pu_up.at(ntrupv_mean));
  else return static_cast<float>(w_pu_nom.at(ntrupv_mean));
}


float weight_tools::triggerEfficiency(int &nmus, int &nels, float &met, vector<float> &sys_trig){
  sys_trig.resize(2,1.);
  int nleps(nmus+nels);

  if(nleps == 0 || nleps > 2) return 1.;

  float eff_trig(1.);
  if(nleps == 1){
    if(nels==1){
      if(met>  0&&met<= 50) eff_trig = 0.897;
      if(met> 50&&met<=100) eff_trig = 0.923;
      if(met>100&&met<=150) eff_trig = 0.958;
      if(met>150&&met<=200) eff_trig = 0.988;
      // 1% systematic for met < 200
      sys_trig[0] = min(1., eff_trig+0.01);
      sys_trig[1] = eff_trig-0.01;
      if(met>200) {
        eff_trig = 0.998;
        sys_trig[0] = min(1., eff_trig+0.005);
        sys_trig[1] = eff_trig-0.005;
      }
    }
    if(nmus==1){
      if(met>  0&&met<= 50) eff_trig = 0.938;
      if(met> 50&&met<=100) eff_trig = 0.943;
      if(met>100&&met<=150) eff_trig = 0.965;
      if(met>150&&met<=200) eff_trig = 0.994;
      // 1% systematic for met < 200
      sys_trig[0] = min(1., eff_trig+0.01);
      sys_trig[1] = eff_trig-0.01;
      if(met>200){
        eff_trig = 0.997;
        sys_trig[0] = min(1., eff_trig+0.005);
        sys_trig[1] = eff_trig-0.005;
      }
    }
  } // nleps == 1

  if(nleps == 2){
    eff_trig = 1.;
    sys_trig[0] = 1.;
    sys_trig[1] = 0.995;
  } // nleps == 2
  return eff_trig;
}

float weight_tools::topPtWeight(float toppt1, float toppt2){
  // 8TeV
  // float pt1,pt2;
  // if(toppt1>400) pt1=400;
  // else pt1=toppt1;
  // if(toppt2>400) pt2=400;
  // else pt2=toppt2;
  //  return sqrt(exp(0.156-0.00137*pt1)*exp(0.156-0.00137*pt2));
  // 13 TeV
  return sqrt(exp(0.0615-0.0005*toppt1)*exp(0.0615-0.0005*toppt2));
}

float weight_tools::isrWeight(float isrpt){
  if(isrpt<400) return 0.;
  else if(isrpt<600) return 0.15;
  else return 0.3;
}

float weight_tools::theoryWeight(weight_tools::variationType variation){
  if(theoryWeights.size()!=0) {
    return theoryWeights.at(variation).wgt/theoryWeights.at(nominal).wgt;
  }
  else return 1.0;
}

void weight_tools::getTheoryWeights(edm::Handle<LHEEventProduct> lhe_info){
  theoryWeights.clear();
  theoryWeights = lhe_info->weights();
}

void weight_tools::getPDFWeights(vector<float> &sys_pdf, vector<float> &w_pdf){
  if(theoryWeights.size()!=0) {
    unsigned ind(10), nweights(100); //index of the first pdf weight and number of replicas
    vector<double> pdfwgt = vector<double>(nweights,1.);
    for (unsigned i(0); i<nweights; i++){
      double ipdfw = theoryWeights[i+ind].wgt/theoryWeights[nominal].wgt;
      w_pdf.push_back(ipdfw);
      pdfwgt[i] = ipdfw;
    }
    auto result = minmax_element(pdfwgt.begin(), pdfwgt.end());
    sys_pdf.push_back(*result.second); //max
    sys_pdf.push_back(*result.first);  //min
  } 
}
