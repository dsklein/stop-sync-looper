#include "analysis.h"
#include "sample.h"
#include "runLooper.h"
#include "sigRegion.h"
#include "sigRegion.cc"

#include "TChain.h"
#include "TFile.h"
#include "TString.h"

#include "CMS3.h"


// Global variables, for use in defining signal regions
extern bool j1_isBtag;
extern double j1pt;

bool j1_isBtag;
double j1pt;

// Help with program options
void printHelp() {
  std::cout << "\nUsage: ./runLooper [arg]\n" << std::endl;

  std::cout << "Takes zero or more arguments from the following list:" << std::endl;
  std::cout << "[blank]     equivalent to 'all'" << std::endl;
  std::cout << "help        show this message" << std::endl;
  std::cout << "all         run ScanChain, makeTables, makeStack, and makeDataCards" << std::endl;
  std::cout << "scan        run ScanChain only" << std::endl;
  std::cout << "plots       run makeStack only" << std::endl;
  std::cout << "tables      run makeTables only" << std::endl;
  std::cout << "cards       run makeDataCards only" << std::endl;
  std::cout << "estimate    run makeLostLepEstimate only" << std::endl;
  std::cout << "output      run makeStack, makeTables, and makeDataCards only" << std::endl;
}

///// MAIN PROGRAM /////
int main( int argc, char* argv[] ) {


  TString bkgPath = "/hadoop/cms/store/user/jgwood/condor/stop_1l_babies/stop_babies__CMS3_V070411__BabyMaker_V0704X_v9__20160127/merged_files/Skims_SR__20160202/";
  TString llepPath = "/hadoop/cms/store/user/jgwood/condor/stop_1l_babies/stop_babies__CMS3_V070411__BabyMaker_V0704X_v9__20160127/merged_files/Skims_CR_2lep__20160202/";

  TString sigPath = "/nfs-7/userdata/stopRun2/signalbabies/";


  ////////////////////////////////////////////////////////////////
  // Parse command-line argument(s)

  std::vector<TString> arguments;
  if( argc==1 )                       arguments.push_back( "all" );
  else  for( int i=1; i<argc; i++ )  arguments.push_back( TString(argv[i]) );

  bool needshelp   = false;
  bool runlooper   = false;
  bool runstacks   = false;
  bool runtables   = false;
  bool runcards    = false;
  bool runlostlep  = false;
  bool runestimate = false;


  for( TString arg : arguments ) {
	if(      arg=="help"  || arg=="h" ) needshelp = true;
	else if( arg=="scan"  || arg=="loop"  || arg=="scanchain" ) runlooper = true;
	else if( arg=="plot"  || arg=="plots" || arg=="stack" || arg=="stacks" ) runstacks = true;
	else if( arg=="table" || arg=="tables" ) runtables = true;
	else if( arg=="cards" || arg=="card"  || arg=="datacards" || arg=="datacard" ) runcards = true;
	else if( arg=="lostlep" || arg=="lost" || arg=="ll" ) runlostlep = true;
	else if( arg=="estimate" || arg=="est" || arg=="bkg" ) runestimate = true;
	else if( arg=="out"   || arg=="output" ) {
	  runstacks = true;
	  runtables = true;
	  runcards  = true;
	}
	else if( arg=="all") {
	  runlooper = true;
	  runstacks = true;
	  runtables = true;
	  runcards  = true;
	  runlostlep = true;
	  runestimate = true;
	}
	else {
	  std::cout << "Unrecognized option: " << arg << std::endl;
	  needshelp = true;
	}
  }

  // If the user inputs a wrong option, print the help and exit
  if( needshelp ) {
	printHelp();
	return 1;
  }


  ////////////////////////////////////////////////////////////////////////////////////////
  // Make "analysis" objects out of "samples"

  analysis* srAnalysis = new analysis( 2.26, "plots.root" );
  analysis* crLostLep  = new analysis( 2.26, "plotsLL.root" );

  //                             new sample( "Label",  "Display name",    TColor,    sampleType )

  // sample* data    = srAnalysis->AddSample( "data",    "Data",           kBlack,    sample::kData );
  sample* signal  = srAnalysis->AddSample( "signal", "T2tt",            kBlue+3,   sample::kSignal );
  signal->AddFile( sigPath + "Signal_T2tt.root" );
  crLostLep->AddSample( signal );

  sample* tt2l    = srAnalysis->AddSample( "tt2l", "$t\\bar{t} \\rightarrow 2l$", "t#bar{t} #rightarrow 2l", kCyan-3,   sample::kBackground );
  sample* tt1l    = srAnalysis->AddSample( "tt1l", "$t\\bar{t} \\rightarrow 1l$", "t#bar{t} #rightarrow 1l", kRed-7,    sample::kBackground );
  sample* singtop = srAnalysis->AddSample( "singletop", "Single Top",   kGreen-4,  sample::kBackground );
  sample* wjets   = srAnalysis->AddSample( "wjets",   "W+Jets",         kOrange-2, sample::kBackground );
  sample* dy      = srAnalysis->AddSample( "dy",      "Drell-Yan",      kRed+2,    sample::kBackground );
  sample* rare    = srAnalysis->AddSample( "rare",    "Rare",           kMagenta-5,sample::kBackground );

  sample* CRdata    = crLostLep->AddSample( "data", "Data",              kBlack,    sample::kData );

  sample* CRttbar   = crLostLep->AddSample( "ttbar", "$t\\bar{t}$", "t#bar{t}", kCyan-3,   sample::kBackground );
  sample* CRsingtop = crLostLep->AddSample( "singletop", "Single Top",   kGreen-4,  sample::kBackground );
  sample* CRwjets   = crLostLep->AddSample( "wjets",   "W+Jets",         kOrange-2, sample::kBackground );
  sample* CRdy      = crLostLep->AddSample( "dy",      "Drell-Yan",      kRed+2,    sample::kBackground );
  sample* CRrare    = crLostLep->AddSample( "rare",    "Rare",           kMagenta-5,sample::kBackground );


  //////////////////////////////////////////////
  // Make selection and signal region objects
  selection<float> MET_250_350( (*tas::pfmet), 250., 350. );
  selection<float> MET_350_450( (*tas::pfmet), 350., 450. );
  selection<float> MET_450_inf( (*tas::pfmet), 450., 9999999. );
  selection<float> MET_250_325( (*tas::pfmet), 250., 325. );
  selection<float> MET_325_inf( (*tas::pfmet), 325., 9999999. );
  selection<float> MET_350_inf( (*tas::pfmet), 350., 9999999. );

  selection<int> nJetsEq2( (*tas::ngoodjets), 2 );
  selection<int> nJetsEq3( (*tas::ngoodjets), 3 );
  selection<int> nJetsGe4( (*tas::ngoodjets), 4, 9999999 );
  selection<int> nJetsGe5( (*tas::ngoodjets), 5, 9999999 );

  selection<float>  lowMT2W( (*tas::MT2W),   0., 200.     );
  selection<float> highMT2W( (*tas::MT2W), 200., 9999999. );

  selection<float> modTop( (*tas::topnessMod), 6.4, 999999. );

  selection<double> j1Pt200( &j1pt, 200., 999999. );
  selection<bool>   j1NoTag( &j1_isBtag, false );


  sigRegion compr250( "compr250", "Compressed, low MET" );
  compr250.addSelections( 3, &MET_250_350, &nJetsEq2, &modTop );

  sigRegion compr350( "compr350", "Compressed, high MET" );
  compr350.addSelections( 3, &MET_350_inf, &nJetsEq2, &modTop );

  sigRegion boost250( "boost250", "Boosted, high MT2W, low MET" );
  boost250.addSelections( 3, &MET_250_350, &nJetsEq3, &highMT2W );

  sigRegion boost350( "boost350", "Boosted, high MT2W, high MET" );
  boost350.addSelections( 3, &MET_350_inf, &nJetsEq3, &highMT2W );

  sigRegion low250( "low250", "Low dM, low MET" );
  low250.addSelections( 3, &lowMT2W, &MET_250_325, &nJetsGe4 );

  sigRegion low325( "low325", "Low dM, high MET" );
  low325.addSelections( 3, &lowMT2W, &MET_325_inf, &nJetsGe4 );

  sigRegion high250( "high250", "High dM, low MET" );
  high250.addSelections( 3, &highMT2W, &MET_250_350, &nJetsGe4 );

  sigRegion high350( "high350", "High dM, mid MET" );
  high350.addSelections( 3, &highMT2W, &MET_350_450, &nJetsGe4 );

  sigRegion high450( "high450", "High dM, high MET" );
  high450.addSelections( 3, &highMT2W, &MET_450_inf, &nJetsGe4 );

  sigRegion inclusive( "inclusive", "Inclusive" );

  sigRegion corridor250( "corridor250", "Corridor, low MET" );
  corridor250.addSelections( 4, &nJetsGe5, &j1Pt200, &j1NoTag, &MET_250_350 );

  sigRegion corridor350( "corridor350", "Corridor, high MET" );
  corridor350.addSelections( 4, &nJetsGe5, &j1Pt200, &j1NoTag, &MET_350_inf );

  std::vector<sigRegion> compressed  = {compr250,  compr350};
  std::vector<sigRegion> boosted     = {boost250,  boost350};
  std::vector<sigRegion> lowDMreg    = {low250,  low325};
  std::vector<sigRegion> highDMreg = {high250, high350, high450};
  std::vector<sigRegion> inclReg = {inclusive};
  std::vector<sigRegion> corridor = {corridor250, corridor350};
  srAnalysis->AddSigRegs( compressed );
  srAnalysis->AddSigRegs( boosted );
  srAnalysis->AddSigRegs( lowDMreg );
  srAnalysis->AddSigRegs( highDMreg );
  srAnalysis->AddSigRegs( inclReg );
  srAnalysis->AddSigRegs( corridor );

  crLostLep->AddSigRegs( compressed );
  crLostLep->AddSigRegs( boosted );
  crLostLep->AddSigRegs( lowDMreg );
  crLostLep->AddSigRegs( highDMreg );
  crLostLep->AddSigRegs( inclReg );
  crLostLep->AddSigRegs( corridor );






  //////////////////////////////////////////////
  // Make chains and run ScanChain

  if( runlooper ) {

	// Data samples

	// data->AddFile( bkgPath + "data_met_2015C_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_met_2015D_05Oct2015_v1_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_met_2015D_promptRecoV4_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_electron_2015C_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_electron_2015D_05Oct2015_v1_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_electron_2015D_promptRecoV4_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_muon_2015C_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_muon_2015D_05Oct2015_v1_25ns_skimmed.root" );
	// data->AddFile( bkgPath + "data_single_muon_2015D_promptRecoV4_25ns_skimmed.root" );

	// Signal samples
	// See above^


	// Background samples

	tt2l->AddFile( bkgPath + "ttbar_powheg_pythia8_25ns_skimmed.root" );

	tt1l->AddFile( bkgPath + "ttbar_powheg_pythia8_25ns_skimmed.root" );

	wjets->AddFile( bkgPath + "WJetsToLNu_HT100To200_madgraph_pythia8_25ns_skimmed.root" );
	wjets->AddFile( bkgPath + "WJetsToLNu_HT200To400_madgraph_pythia8_25ns_skimmed.root" );
	wjets->AddFile( bkgPath + "WJetsToLNu_HT400To600_madgraph_pythia8_25ns_skimmed.root" );
	wjets->AddFile( bkgPath + "WJetsToLNu_HT600ToInf_madgraph_pythia8_25ns_skimmed.root" );

	dy->AddFile( bkgPath + "DYJetsToLL_m10To50_amcnlo_pythia8_25ns_skimmed.root" );
	dy->AddFile( bkgPath + "DYJetsToLL_m50_amcnlo_pythia8_25ns_skimmed.root" );

	singtop->AddFile( bkgPath + "t_sch_4f_amcnlo_pythia8_25ns_skimmed.root" );
	singtop->AddFile( bkgPath + "t_tch_4f_powheg_pythia8_25ns_skimmed.root" );
	singtop->AddFile( bkgPath + "tbar_tch_4f_powheg_pythia8_25ns_skimmed.root" );
	singtop->AddFile( bkgPath + "t_tW_5f_powheg_pythia8_25ns_skimmed.root" );
	singtop->AddFile( bkgPath + "t_tbarW_5f_powheg_pythia8_25ns_skimmed.root" );

	rare->AddFile( bkgPath + "TTWJetsToLNu_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "TTWJetsToQQ_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "TTZToLLNuNu_M-10_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "TTZToQQ_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "tZq_ll_4f_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "tZq_nunu_4f_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "WWTo2l2Nu_powheg_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "WWToLNuQQ_powheg_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "WZTo3LNu_powheg_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "WZTo2L2Q_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "WZTo1LNu2Q_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "ZZTo4L_powheg_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "ZZTo2L2Q_amcnlo_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "ZZTo2L2Nu_powheg_pythia8_25ns_skimmed.root" );
	rare->AddFile( bkgPath + "ZZTo2Q2Nu_amcnlo_pythia8_25ns_skimmed.root" );



	// Reset output files
	TFile* outfile = new TFile( srAnalysis->GetFileName(), "RECREATE");
	outfile->Close();

	// Run ScanChain on all samples
	for( sample* mySample : srAnalysis->GetAllSamples() ) ScanChain( srAnalysis, mySample );

  }

  //////////////////////////////////////////////
  // Run lost lepton background estimate

  if( runlostlep ) {

	CRdata->AddFile( llepPath + "data_met_2015C_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_met_2015D_05Oct2015_v1_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_met_2015D_promptRecoV4_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_electron_2015C_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_electron_2015D_05Oct2015_v1_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_electron_2015D_promptRecoV4_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_muon_2015C_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_muon_2015D_05Oct2015_v1_25ns_skimmed.root" );
	CRdata->AddFile( llepPath + "data_single_muon_2015D_promptRecoV4_25ns_skimmed.root" );
	CRttbar->AddFile( llepPath + "ttbar_powheg_pythia8_25ns_skimmed.root" );
	CRwjets->AddFile( llepPath + "WJetsToLNu_HT100To200_madgraph_pythia8_25ns_skimmed.root" );
	CRwjets->AddFile( llepPath + "WJetsToLNu_HT200To400_madgraph_pythia8_25ns_skimmed.root" );
	CRwjets->AddFile( llepPath + "WJetsToLNu_HT400To600_madgraph_pythia8_25ns_skimmed.root" );
	CRwjets->AddFile( llepPath + "WJetsToLNu_HT600ToInf_madgraph_pythia8_25ns_skimmed.root" );
	CRdy->AddFile( llepPath + "DYJetsToLL_m10To50_amcnlo_pythia8_25ns_skimmed.root" );
	CRdy->AddFile( llepPath + "DYJetsToLL_m50_amcnlo_pythia8_25ns_skimmed.root" );
	CRsingtop->AddFile( llepPath + "t_sch_4f_amcnlo_pythia8_25ns_skimmed.root" );
	CRsingtop->AddFile( llepPath + "t_tch_4f_powheg_pythia8_25ns_skimmed.root" );
	CRsingtop->AddFile( llepPath + "tbar_tch_4f_powheg_pythia8_25ns_skimmed.root" );
	CRsingtop->AddFile( llepPath + "t_tW_5f_powheg_pythia8_25ns_skimmed.root" );
	CRsingtop->AddFile( llepPath + "t_tbarW_5f_powheg_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "TTWJetsToLNu_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "TTWJetsToQQ_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "TTZToLLNuNu_M-10_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "TTZToQQ_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "tZq_ll_4f_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "tZq_nunu_4f_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "WWTo2l2Nu_powheg_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "WWToLNuQQ_powheg_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "WZTo3LNu_powheg_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "WZTo2L2Q_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "WZTo1LNu2Q_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "ZZTo4L_powheg_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "ZZTo2L2Q_amcnlo_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "ZZTo2L2Nu_powheg_pythia8_25ns_skimmed.root" );
	CRrare->AddFile( llepPath + "ZZTo2Q2Nu_amcnlo_pythia8_25ns_skimmed.root" );

	TFile* outfile = new TFile( crLostLep->GetFileName(), "RECREATE");
	outfile->Close();

	// Run lost lepton CR looper on all samples
	for( sample* mySample : crLostLep->GetAllSamples() ) looperCR2lep( crLostLep, mySample );

  }


  /////////////////////////////////////////////////////
  // Make stacked histograms and/or yield tables

  if( runtables   ) makeTables( srAnalysis );
  if( runlostlep  ) makeTables( crLostLep  );
  if( runstacks   ) makeStack( srAnalysis );
  if( runestimate ) makeLostLepEstimate( srAnalysis, crLostLep );
  if( runcards    ) makeDataCards( srAnalysis );

  return 0;
}
