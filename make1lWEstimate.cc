#include <iostream>

#include "analysis.h"
#include "sample.h"
#include "systematic.h"

#include "TFile.h"
#include "TH1.h"
#include "TH2D.h"

using namespace std;

void do1lWestimate( TFile* srhistfile, TFile* crhistfile, TString systSuffix );

// Declare some static global variables
// This simplifies the process of repeating the background estimate over multiple systematic variations
static vector<TString> srnames;
static vector<TString> crnames;
static vector<TString> crBkgLabels;
static uint nSRegions;
static uint nCRegions;
static TH1D* h_crData = NULL;
static bool crHasSignal = false;
static vector< vector<sigRegion> > sigRegionList;



// Main function ////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------//
void make1lWEstimate( analysis* srAnalysis, analysis* crAnalysis ) {

	TH1::SetDefaultSumw2();

	// Open input files and output file
	TFile* srHistFile = new TFile( srAnalysis->GetPlotFileName(), "READ" );
	TFile* crHistFile = new TFile( crAnalysis->GetPlotFileName(), "READ" );
	TFile* srSystFile = new TFile( srAnalysis->GetSystFileName(), "READ" );
	TFile* crSystFile = new TFile( crAnalysis->GetSystFileName(), "READ" );
	TFile* srJesFile  = new TFile( "jes_sr.root",   "READ" );
	TFile* crJesFile  = new TFile( "jes_cr0b.root", "READ" );

	// Check for bad files
	for( TFile* thisFile : {srHistFile,crHistFile,srSystFile,crSystFile,srJesFile,crJesFile} ) {
		if( thisFile->IsZombie() ) {
			cout << "Error in make1lWEstimate! Couldn't open file " << thisFile->GetName() << "!" << endl;
			return;
		}
	}
	TFile* outFile    = new TFile( "onelepwEstimates.root",   "RECREATE" );
	outFile->cd();

	// Set the values of our global variables
	srnames = srAnalysis->GetSigRegionLabelsAll();
	crnames = crAnalysis->GetSigRegionLabelsAll();
	nSRegions = srnames.size();
	nCRegions = crnames.size();
	if( nSRegions != nCRegions ) {
		cout << "Error in make1lWestimate: Different number of signal and control regions!" << endl;
		return;
	}
	if( crAnalysis->HasData() ) h_crData = (TH1D*)crHistFile->Get("srYields_"+crAnalysis->GetData()->GetLabel())->Clone("crData");
	crHasSignal = ( crAnalysis->GetNsignals() >= 1 );
	crBkgLabels = crAnalysis->GetBkgLabels();
	sigRegionList = srAnalysis->GetSigRegions();


	// Now actually run the 1l-from-W background estimates!
	// Once for the nominal estimate, and once for each of the systematic variations
	do1lWestimate( srHistFile, crHistFile, "" );
	for( systematic* thisSys : srAnalysis->GetSystematics(true) ) {
		TString suffix = "_" + thisSys->GetNameLong();
		if( suffix.Contains("JES") ) do1lWestimate( srJesFile, crJesFile, suffix );
		else do1lWestimate( srSystFile, crSystFile, suffix );
	}


	// Clean up
	outFile->Close();
	crSystFile->Close();
	srSystFile->Close();
	crHistFile->Close();
	srHistFile->Close();

	delete outFile;
	delete crSystFile;
	delete srSystFile;
	delete crHistFile;
	delete srHistFile;

}



// Function that actually does the nitty-gritty of the 1l-from-W background estimate /////////////////
void do1lWestimate( TFile* srhistfile, TFile* crhistfile, TString systSuffix ) {

	// Define histograms that will hold the SR/CR yields
	TH1D* h_srMC   = new TH1D( "srMC"  , "Signal region yields from MC"   , nSRegions, 0.5, float(nSRegions)+0.5 );
	TH1D* h_crMC   = new TH1D( "crMC"  , "Control region yields from MC"  , nCRegions, 0.5, float(nCRegions)+0.5 );
	for( uint i=0; i<nSRegions; i++ ) h_srMC->GetXaxis()->SetBinLabel( i+1, srnames.at(i) );
	for( uint i=0; i<nCRegions; i++ ) h_crMC->GetXaxis()->SetBinLabel( i+1, crnames.at(i) );


	// Get 1l-from-W background yields from MC in signal regions
	for( uint i=0; i<srnames.size(); i++ ) {
		TH1D* histo = (TH1D*)srhistfile->Get("evttype_"+srnames.at(i)+systSuffix);
		if( histo == 0 ) {
			cout << "Error in make1lWestimate! Could not find histogram evttype_" << srnames.at(i)+systSuffix << " in file " << srhistfile->GetName() << "!" << endl;
			return;
		}
		h_srMC->SetBinContent(i+1, histo->GetBinContent(6));
		h_srMC->SetBinError(i+1, histo->GetBinError(6));
	}

	// Get total yields from MC in 0-btag control regions
	for( uint i=0; i<crnames.size(); i++ ) {
		TH1D* histo = (TH1D*)crhistfile->Get("evttype_"+crnames.at(i)+systSuffix);
		if( histo == 0 ) {
			cout << "Error in make1lWestimate! Could not find histogram evttype_" << crnames.at(i)+systSuffix << " in file " << crhistfile->GetName() << "!" << endl;
			return;
		}
		double yield, error;
		yield = histo->IntegralAndError( 3, 6, error );
		h_crMC->SetBinContent( i+1, yield );
		h_crMC->SetBinError(   i+1, error );
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Need to make the low MT2W estimates come directly from MC
	//

	// Now do the division, M^SR / M^CR
	TH1D* h_mcRatio = (TH1D*)h_srMC->Clone("mcRatio1lepW"+systSuffix);
	h_mcRatio->SetTitle( "SR/CR ratio by signal region" );
	for( uint i=0; i<nSRegions; i++ ) h_crMC->GetXaxis()->SetBinLabel( i+1, srnames.at(i) ); // equalize bin names
	h_mcRatio->Divide( h_crMC );


	// Get data yields in CRs, and multiply by M/M
	TH1D* h_bkgEstimate;

	if( h_crData == NULL ) {  // If we don't have CR data, do a dummy estimate from MC
		h_crData = (TH1D*)h_crMC->Clone("crData");
		cout << "\nWarning in make1lWestimate.cc: No data sample found in control region. Using MC as a dummy instead." << endl;
	}
	h_crData->SetTitle( "Control region yields from data" );
	if( systSuffix == "" ) h_crData->Write();
	for( uint i=0; i<nSRegions; i++ ) h_crData->GetXaxis()->SetBinLabel( i+1, srnames.at(i) ); // equalize bin names

	TString histname = systSuffix=="" ? "onelepwBkg" : "variation"+systSuffix;
	h_bkgEstimate = (TH1D*)h_crData->Clone( histname );
	h_bkgEstimate->SetTitle( "1l-from-W background estimate" );
	h_bkgEstimate->Multiply( h_mcRatio );

	// Write everything to a file
	h_mcRatio->Write();
	h_bkgEstimate->Write();
	if( systSuffix == "" ) cout << "1l-from-W background estimate saved in " << gFile->GetName() << "." << endl;
	else cout << "Systematic variation " << systSuffix << " saved in " << gFile->GetName() << "." << endl;

	delete h_srMC;
	delete h_crMC;

	if( systSuffix != "" ) return;

	////////////////////////////////////////////////////////////////
	// Now do some calculations for the systematics...

	// Calculate the signal contamination in the CRs
	if( crHasSignal ) {
		for( uint i=0; i<nCRegions; i++ ) {
			double ratio = h_mcRatio->GetBinContent( i+1 );
			double ratio_err = h_mcRatio->GetBinError( i+1 );
			TH2D* h_contam = (TH2D*)crhistfile->Get("sigyields_"+crnames.at(i)+systSuffix)->Clone("sigContam_"+srnames.at(i));
			for( int bin=0; bin<h_contam->GetNcells(); bin++ ) {
				double yield = h_contam->GetBinContent(bin);
				double error = h_contam->GetBinError(bin);
				if( yield < 0.000001 ) continue;
				h_contam->SetBinContent( bin, yield*ratio );
				h_contam->SetBinError( bin, sqrt( yield*yield*ratio_err*ratio_err + ratio*ratio*error*error ) ); // Gaussian error propagation
			}
			h_contam->Write();
		}
		cout << "Signal contamination estimate saved in " << gFile->GetName() << "." << endl;
	}

	// Isolate the uncertainties due to signal stats and MC stats
	TH1D* h_datastats = (TH1D*)h_mcRatio->Clone("estimate_datastats");
	TH1D* h_mcstats   = (TH1D*)h_crData->Clone("estimate_mcstats");
	h_datastats->SetTitle( "Background estimate with uncertainty from data stats only" );
	h_mcstats->SetTitle( "Background estimate with uncertainty from MC stats only" );
	for( uint i=1; i<=nSRegions; i++ ) {
		h_datastats->SetBinError(i, 0.);
		h_mcstats->SetBinError(i, 0.);
	}
	h_datastats->Multiply( h_crData );
	h_mcstats->Multiply( h_mcRatio );
	h_datastats->Write();
	h_mcstats->Write();


	////////////////////////////////////////////////////////////////////
	// Print out a table with the details of the 1l-from-W estimate

	//  Print table header
	cout << "\n1l-from-W background estimate (stat errors only)\n" << endl;
	cout << "\\begin{tabular}{ | l | c | c | c | }" << endl;
	cout << "\\hline" << endl;
	cout << "Signal region  &  $N^{CR}$  &  Transfer factor  &  $N_{est}^{SR}$ \\\\" << endl;
	cout << "\\hline" << endl;

	// Loop through signal regions and print out table rows
	uint binOffset = 1;
	for( vector<sigRegion> sigRegs : sigRegionList ) {
		for( uint i=0; i<sigRegs.size(); i++ ) {

			printf( "%30s & %3d $\\pm$ %5.3f &  %5.3f $\\pm$ %5.3f  &  %5.2f $\\pm$ %5.2f  \\\\\n", sigRegs.at(i).GetTableName().Data(),
			        int(h_crData->GetBinContent(i+binOffset)), h_crData->GetBinError(i+binOffset), h_mcRatio->GetBinContent(i+binOffset),
			        h_mcRatio->GetBinError(i+binOffset), h_bkgEstimate->GetBinContent(i+binOffset), h_bkgEstimate->GetBinError(i+binOffset) );

		}
		binOffset += sigRegs.size();
		cout << "\\hline" << endl;
	}
	cout << "\\end{tabular}\n" << endl;

}