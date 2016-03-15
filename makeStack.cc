#include "dataMCplotMaker.h"
#include "analysis.h"
#include "sample.h"

#include "TFile.h"
// #include "TColor.h"

#include <cmath>

using namespace std;

void makeStack( analysis* myAnalysis) {

  TFile* plotfile = new TFile("plots.root", "READ");

  vector<TString> varNames;       vector<TString> axisLabels; //variables
  varNames.push_back("mt"     );  axisLabels.push_back("M_{T}");
  varNames.push_back("met"    );  axisLabels.push_back("MET");
  varNames.push_back("mt2w"   );  axisLabels.push_back("MT2W");
  varNames.push_back("chi2"   );  axisLabels.push_back("#chi^{2}");
  varNames.push_back("htratio");  axisLabels.push_back("H_{T} ratio");
  varNames.push_back("mindphi");  axisLabels.push_back("#Delta#phi");
  varNames.push_back("ptb1"   );  axisLabels.push_back("p_{T}");
  varNames.push_back("drlb1"  );  axisLabels.push_back("#DeltaR");
  varNames.push_back("ptlep"  );  axisLabels.push_back("p_{T}");
  varNames.push_back("metht"  );  axisLabels.push_back("MET/sqrt H_{T}");
  varNames.push_back("dphilw" );  axisLabels.push_back("#Delta#phi");
  varNames.push_back("bkgtype");  axisLabels.push_back("Background type");
  varNames.push_back("njets"  );  axisLabels.push_back("Number of jets");
  varNames.push_back("nbtags" );  axisLabels.push_back("Number of b-tags");
  
  vector<TString> regNames = myAnalysis->GetSigRegionsAll(); //signal region names

  // Loop over all the variables we're plotting
  for( uint j=0; j<regNames.size(); j++ ) {
	for( uint i=0; i<varNames.size(); i++ ) {

	  vector<TH1F*> bkgs;
	  vector<TH1F*> sigs;
	  TH1F* data;

	  // Retrieve the histograms for each background
	  for( TString sampleName : myAnalysis->GetBkgNamesStorage() ) {
		TString plotname = varNames.at(i) + "_" + sampleName + "_" + regNames.at(j);
		TH1F*   histo    = (TH1F*)plotfile->Get(plotname);
		bkgs.push_back(histo);
	  }

	  // Retrieve the histograms for each signal
	  for( TString sampleName : myAnalysis->GetSignalNamesStorage() ) {
		TString plotname = varNames.at(i) + "_" + sampleName + "_" + regNames.at(j);
		TH1F*   histo    = (TH1F*)plotfile->Get(plotname);
		sigs.push_back(histo);
	  }

	  // If there's a data sample, get the histogram
	  if( myAnalysis->HasData() ) {
		TString plotname = varNames.at(i) + "_" + myAnalysis->GetData()->GetIntName() + "_" + regNames.at(j);
		data = (TH1F*)plotfile->Get(plotname);
	  }
	  else data = new TH1F("", "", 1, 0, 1);

	  // Get sample titles and colors from the "analysis" object
	  vector<string> bkg_titles = myAnalysis->GetBkgNamesLegend();
	  vector<string> sig_titles = myAnalysis->GetSignalNamesLegend();
	  vector<short int> colors  = myAnalysis->GetColors();

	  // Get the title and subtitle for the plot
	  TString plotTitle = bkgs.at(0)->GetTitle();
	  TString plotSubTitle = "Region: " + regNames.at(j);


	  // Make the options string for each stack
	  TString optString = "--energy 13 --lumi 5 --xAxisLabel "+axisLabels.at(i)+" --xAxisUnit --outputName plots/stack_" + varNames.at(i) + "_" + regNames.at(j); // + " --png";
	  // (try also --isLinear);  --legendTextSize 0.022

	  // Run the big tamale...
	  dataMCplotMaker( data,
					   bkgs,
					   bkg_titles,
					   plotTitle.Data(), //title
					   plotSubTitle.Data(), //subtitle
					   optString.Data(), //options
					   sigs,
					   sig_titles,
					   colors );

	} // End loop over variables to plot
  } // End loop over signal regions

  plotfile->Close();
  delete plotfile;

}
