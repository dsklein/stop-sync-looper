#include <iostream>
#include <fstream>

#include "TFile.h"
#include "TH1.h"
#include "TH2D.h"

#include "analysis.h"
#include "sample.h"

using namespace std;



void makeDataCards( analysis* myAnalysis ) {

  // Do the basic setup stuff

  vector<TString> bkgs = { "zNuNu", "dilep", "top1l", "W1l" }; // Eventually pull this from the analysis object
  const int nBkgs = bkgs.size();

  if( myAnalysis->GetNsignals() < 1 ) {
	cout << "\nError in makeDataCards.cc: Need at least one signal sample!" << endl;
	return;
  }

  vector<TString> samples = bkgs;
  samples.insert( samples.begin(), "signal" );
  const int nSamples = samples.size();

  vector<TString> sigRegions = myAnalysis->GetSigRegionsAll();
  const int nSigRegs = sigRegions.size();

  // Open files containing background yields and uncertainties
  TFile* yieldFile   = new TFile( myAnalysis->GetFileName(), "READ" );
  TFile* lostlepFile = new TFile( "bkgEstimates.root", "READ" );
  TH1D* h_lostLep = (TH1D*)lostlepFile->Get("lostLepBkg");
  
  //////////////////////////////////////////////////////////////////////////////
  // Loop over signal regions, making a datacard for each SR and each mass point
  //////////////////////////////////////////////////////////////////////////////

  for( int reg=1; reg<=nSigRegs; reg++ ) {

	TH1D* h_bkgYield  = (TH1D*)yieldFile->Get( "evttype_"+sigRegions.at(reg-1) );
	TH2D* h_sigYield  = (TH2D*)yieldFile->Get( "sigyields_"+sigRegions.at(reg-1) );
	TH2D* h_sigContam = (TH2D*)lostlepFile->Get( "sigContam_"+sigRegions.at(reg-1) );

	// Subtract signal contamination from signal MC prediction
	// Had a LONG discussion with John and FKW about why this is the appropriate way to do it
	h_sigYield->Add( h_sigContam, -1. );

	// Loop over signal mass points
	for( int xbin=1; xbin<=h_sigYield->GetNbinsX(); xbin++ ) {
	  for( int ybin=1; ybin<=h_sigYield->GetNbinsY(); ybin++ ) {

		int stopmass = int( h_sigYield->GetXaxis()->GetBinCenter(xbin) );
		int lspmass  = int( h_sigYield->GetYaxis()->GetBinCenter(ybin) );

		double sigYield = h_sigYield->GetBinContent( xbin, ybin );
		double sigError = h_sigYield->GetBinError( xbin, ybin );
		if( sigYield < 0.000001 ) continue;


		// Do some acrobatics to send the output to a file...
		TString fileName = Form( "datacards/datacard_%s_T2tt_%d_%d.txt", sigRegions.at(reg-1).Data(), stopmass, lspmass );
		cout << "Writing data card " << fileName << endl;

		FILE * outfile;
		outfile = fopen( fileName.Data(), "w" );


		////////////////////////////
		// Now print the data card

		fprintf( outfile,  "# Data card for signal region %d (%s)\n", reg, sigRegions.at(reg-1).Data() );
		fprintf( outfile,  "# Stop mass = %d, LSP mass = %d\n", stopmass, lspmass );
		fprintf( outfile,  "---\n" );
		fprintf( outfile,  "imax 1  number of channels\n" );

		fprintf( outfile,  "jmax %d  number of backgrounds (", nBkgs );
		for (TString bkgName : bkgs ) fprintf( outfile, "%s, ",  bkgName.Data() );   // Will usually be 4 (znunu, 2l, 1ltop, 1lw)
		fprintf( outfile,  ")\n" );

		fprintf( outfile,  "kmax %d  number of uncertainties\n", 2*nSamples-1 ); // For now, we've got statistical and systematic for each bkg, and stat for signal
		fprintf( outfile,  "---\n" );

		fprintf( outfile,  "# Now list the number of events observed (or zero if no data)\n" );
		fprintf( outfile,  "bin %d\n", reg );
		if( myAnalysis->HasData() ) fprintf( outfile,  "observation %d\n", int(h_bkgYield->GetBinContent(1)) );
		else                        fprintf( outfile,  "observation 0\n" );
		fprintf( outfile,  "---\n" );

		fprintf( outfile,  "# Now list the expected events (i.e. Monte Carlo yield) for signal and all backgrounds in our particular bin\n" );
		fprintf( outfile,  "# Second 'process' line should be zero for signal, and a positive integer for each background\n" );

		fprintf( outfile,  "bin      ");
		for( TString sample : samples ) fprintf( outfile,  " %10i", reg );  // Print the signal region number once for each sample (signal and bkg)
		fprintf( outfile, "\n" );

		fprintf( outfile,  "process  ");
		for( TString sample : samples ) fprintf( outfile,  " %10s", sample.Data() );  // Print the name of each sample (sig & bkg)
		fprintf( outfile, "\n" );

		fprintf( outfile,  "process  ");
		for( int j=0; j<nSamples; j++ ) fprintf( outfile,  " %10i", j ); // Print a number for each sample (0=signal, positive integers for bkgs)
		fprintf( outfile, "\n" );



		// Print row with the yields for each process (signal, bkgs)
		fprintf( outfile,  "rate     ");
		double yield;

		for( int i=1; i<=nSamples; i++ ) {
		  if(      i==1 ) yield = sigYield;
		  else if( i==3 ) yield = h_lostLep->GetBinContent(reg); // Pull 2l yield from lostLepton estimate histogram
		  else            yield = h_bkgYield->GetBinContent(i+1);
		  fprintf( outfile,  " %10f", yield );
		}
		fprintf( outfile, "\n" );

		fprintf( outfile,  "---\n" );
		fprintf( outfile,  "# Now we list the independent sources of uncertainty (syst. and stat. error), and which samples they affect\n" );
		fprintf( outfile,  "---\n" );



		// Print out a row for the statistical uncertainty on each sample
		for( int sampleIdx=0; sampleIdx<nSamples; sampleIdx++ ) {

		  char statname[25];
		  sprintf( statname, "%sStat%d", samples.at(sampleIdx).Data(), reg );	  
		  fprintf( outfile,   "%-18s  lnN ", statname );

		  double statErr = 1.0 + (h_bkgYield->GetBinError(sampleIdx+2) / h_bkgYield->GetBinContent(sampleIdx+2) );
		  if( sampleIdx==0 ) statErr = 1.0 + ( sigError / sigYield ); // Pull stat error on signal from special signal yield histogram
		  if( sampleIdx==2 ) statErr = 1.0 + ( h_lostLep->GetBinError(reg) / h_lostLep->GetBinContent(reg) );  // Pull 2l stat error from lostLepton estimate histogram

		  for( int j=0; j<nSamples; j++ ) {
			if( j == sampleIdx )  fprintf( outfile, "  %8.6f  ", statErr);
			else fprintf( outfile,  "     -      " );
		  }
		  fprintf( outfile, "\n" );
		}


		// Print out a row for the systematic uncertainty on each sample
		for( int sampleIdx = 1; sampleIdx<nSamples; sampleIdx++ ) { // For now, skip the signal sample (don't give it a systematic uncertainty)

		  char systname[25];
		  sprintf( systname, "%sSyst", samples.at(sampleIdx).Data() );	  
		  fprintf( outfile,   "%-18s  lnN ", systname );

		  double systErr = 1.3; // Flat 30% systematic for now
		  // if( sampleIdx = 4 ) systErr = 2.0; // 100% systematic on 1-lepton from top

		  for( int j=0; j<nSamples; j++ ) {
			if( j == sampleIdx )  fprintf( outfile, "  %8.6f  ", systErr);
			else fprintf( outfile,  "     -      " );
		  }
		  fprintf( outfile, "\n" );
		}


		fprintf( outfile,  "---\n" );
		fclose(outfile);

	  } // End loop over y bins (LSP masses)
	} // End loop over x bins (stop masses)

  } // End loop over signal regions

  lostlepFile->Close();
  yieldFile->Close();
  delete lostlepFile;
  delete yieldFile;
}
