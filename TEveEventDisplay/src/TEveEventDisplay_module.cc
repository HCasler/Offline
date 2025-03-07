//... libCore
#include <TApplication.h>
#include <TSystem.h>
#include <TList.h>
#include <TObjArray.h>
#include <Rtypes.h>

// ... libRIO
#include <TFile.h>

//TEveEventDisplay Headers:
#include  "Offline/TEveEventDisplay/src/TEveMu2e_base_classes/TEveMu2eMainWindow.h"
#include  "Offline/TEveEventDisplay/src/dict_classes/Collection_Filler.h"
#include  "Offline/TEveEventDisplay/src/dict_classes/Data_Collections.h"
#include  "Offline/TEveEventDisplay/src/DisplayUtilsConfig.hh"

// Framework includes.
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art_root_io/TFileService.h"

//fcl
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/TableAs.h"

using namespace std;
using namespace mu2e;

namespace mu2e
{
  class TEveEventDisplay : public art::EDAnalyzer {
    public:

      struct Config{
        using Name=fhicl::Name;
        using Comment=fhicl::Comment;
        fhicl::Atom<int> diagLevel{Name("diagLevel"), Comment("for info"),0};
        fhicl::Atom<bool> isMCOnly{Name("isMCOnly"), Comment(""),false};
        fhicl::Atom<bool> accumulate{Name("accumulate"), Comment(""),false};
        fhicl::Table<Collection_Filler::Config> filler{Name("filler"),Comment("fill collections")};
        fhicl::Atom<std::string>gdmlname{Name("gdmlname"),Comment("gdmlname")};
        fhicl::Sequence<int>particles{Name("particles"),Comment("PDGcodes to plot")};
        fhicl::TableAs<TEveMu2eMainWindow::GeomOptions,DisplayUtilsConfig::ShowConfig> show {Name("show"), Comment("Control which view to show")};
      };

      typedef art::EDAnalyzer::Table<Config> Parameters;
            explicit TEveEventDisplay(const Parameters& conf);
      virtual ~TEveEventDisplay();
      virtual void beginJob() override;
      virtual void beginRun(const art::Run& run) override;
      virtual void analyze(const art::Event& e);
      virtual void endJob() override;

    private:
      Config _conf;
      int _diagLevel;
      bool _isMCOnly;
      bool _accumulate;
      TApplication* application_;
      TDirectory*   directory_ = nullptr;
      Collection_Filler _filler;
      std::string _gdmlname;
      std::vector<int> _particles;
      const TEveMu2eMainWindow::GeomOptions _show;
      TEveMu2eMainWindow *_frame;
      fhicl::ParameterSet _pset;
      bool foundEvent = false;
      void MakeTEveMu2eMainWindow();
      bool _firstLoop = true;
      int runn = 0;
      int subrunn= 0;
      int eventn = 0;
      bool eventSelected = false;
  };

  TEveEventDisplay::TEveEventDisplay(const Parameters& conf) :
  art::EDAnalyzer(conf),
  _diagLevel(conf().diagLevel()),
  _isMCOnly(conf().isMCOnly()),
  _accumulate(conf().accumulate()),
  _filler(conf().filler()),
  _gdmlname(conf().gdmlname()),
  _particles(conf().particles()),
  _show(conf().show())
  {}


  TEveEventDisplay::~TEveEventDisplay(){}

  void TEveEventDisplay::beginJob(){

    directory_ = gDirectory;
    if ( !gApplication ){
      int    tmp_argc(0);
      char** tmp_argv(0);
      application_ = new TApplication( "noapplication", &tmp_argc, tmp_argv );
    }
    //construct GUI:
    const DrawOptions DrawOpts(_filler.addCrvHits_, _filler.addCosmicSeedFit_, _filler.addTracks_, _filler.addClusters_, _filler.addHits_, _filler.addTrkHits_, _filler.addTimeClusters_, false, _filler.addMCTraj_, _filler.addKKTracks_);

    _frame = new TEveMu2eMainWindow(gClient->GetRoot(), 1000,600, _pset, DrawOpts, _show);

    //build 2D geometries (now optional):
    if(DrawOpts.addCRVInfo)_frame->CreateCRVProjection();
    if(DrawOpts.addClusters or DrawOpts.addCryHits) _frame->CreateCaloProjection();
    _frame->CreateTrackerProjection();
    //send list of particles to viewer:
    _frame->SetParticleOpts(_particles);

  }

  void TEveEventDisplay::beginRun(const art::Run& run){
    //import 3D GDML geom:
    _frame->SetRunGeometry(run, _gdmlname, _diagLevel);
    //make 2D tracker and calo:
    if(_filler.addClusters_) _frame->PrepareCaloProjectionTab(run);
    _frame->PrepareTrackerProjectionTab(run);
  }

  void TEveEventDisplay::analyze(const art::Event& event){
    std::cout<<"[In TEveEventDisplay::analyze()]"<<std::endl;
    int eventid = event.id().event();
    int runid = event.run();
    int subrunid = event.subRun();

    if((eventSelected==false) or ( eventSelected == true and runid == runn and eventid == eventn)){
      std::cout<<"Drawing Run : "<<runid<<" Sub-Run "<<subrunid<<" Event : "<<eventid<<std::endl;
      foundEvent = true;
      Data_Collections data;
      if(_filler.addHits_)_filler.FillRecoCollections(event, data, ComboHits);
      if(_filler.addTimeClusters_)_filler.FillRecoCollections(event, data, TimeClusters);
      if(_filler.addTrkHits_)_filler.FillRecoCollections(event, data, ComboHits);
      if(_filler.addCrvHits_)_filler.FillRecoCollections(event, data, CRVRecoPulses);
      if(_filler.addCosmicSeedFit_)_filler.FillRecoCollections(event, data, CosmicTracks);
      if(_filler.addTracks_)_filler.FillRecoCollections(event, data, KalSeeds);
      if(_filler.addClusters_)_filler.FillRecoCollections(event, data, CaloClusters);
      if(_filler.addMCTraj_)_filler.FillMCCollections(event, data, MCTrajectories);
      if(!_frame->isClosed()) _frame->setEvent(event,  _firstLoop, data, -1, _accumulate, runn, eventn, eventSelected, _isMCOnly);
      _firstLoop = false;

    }
  }


  void TEveEventDisplay::endJob(){
    if(!foundEvent){
      char msg[300];
      sprintf(msg, "Reached end of file but #%i has not been found", true);
      new TGMsgBox(gClient->GetRoot(), gClient->GetRoot(), "Event Not Found", msg, kMBIconExclamation,kMBOk);
    }
  }

}
using mu2e::TEveEventDisplay;
DEFINE_ART_MODULE(TEveEventDisplay);

