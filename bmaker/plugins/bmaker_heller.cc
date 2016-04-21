//// BMAKER_HELLER: Creates baby tree with heller branches
//// Function names follow the first-lowercase, following words-uppercase. No underscores

// System include files
#include <cmath>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include <iomanip>  // put_time

// FW include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// FW physics include files
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

// ROOT include files
#include "TFile.h"
#include "TROOT.h"

// User include files
#include "babymaker/bmaker/interface/bmaker_heller.hh"
#include "babymaker/bmaker/interface/baby_heller.hh"
#include "babymaker/bmaker/interface/cross_sections.hh"

using namespace std;
using namespace utilities;

///////////////////////// analyze: METHOD CALLED EACH EVENT ///////////////////////////
void bmaker_heller::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  nevents++;
  isData = iEvent.isRealData();
  baby.Clear();

  ////////////////////// Event info /////////////////////
  baby.run() = iEvent.id().run();
  baby.event() = iEvent.id().event();
  baby.lumiblock() = iEvent.luminosityBlock();
  if(isData){
    if (debug) cout<<"INFO: Checking JSON..."<<endl;
    // We are applying the golden JSON with lumisToProcess in bmaker_heller_cfg.py
    bool nonblind(eventTool->isInJSON("nonblind", baby.run(), baby.lumiblock()));
    //if(!isInJSON("golden", baby.run(), baby.lumiblock()) && !nonblind) return;
    baby.nonblind() = nonblind;
  } else baby.nonblind() = true;

  ////////////////////// Trigger /////////////////////
  if (debug) cout<<"INFO: Processing trigger info..."<<endl;
  bool triggerFired;
  edm::Handle<edm::TriggerResults> triggerBits;
  iEvent.getByLabel(edm::InputTag("TriggerResults","","HLT"),triggerBits);  
  edm::Handle<pat::PackedTriggerPrescales> triggerPrescales;
  iEvent.getByLabel("patTrigger",triggerPrescales);  

  if(triggerBits.isValid() && triggerPrescales.isValid()){
    const edm::TriggerNames &names = iEvent.triggerNames(*triggerBits);
    // Not saving data events that don't have triggers we care about
    triggerFired = writeTriggers(names, triggerBits, triggerPrescales);
  }
  else
    triggerFired = true;
  if(!triggerFired && isData) {
    reportTime(iEvent);
    return;
  }

  ////////////////////// Primary vertices /////////////////////
  if (debug) cout<<"INFO: Writing vertices..."<<endl;
  edm::Handle<reco::VertexCollection> vtx;
  iEvent.getByLabel("offlineSlimmedPrimaryVertices", vtx);
  edm::Handle<std::vector< PileupSummaryInfo > >  pu_info;
  if(!isData) {
    iEvent.getByLabel("addPileupInfo", pu_info);
    if(!pu_info.isValid()) iEvent.getByLabel("slimmedAddPileupInfo", pu_info);
  }
  writeVertices(vtx, pu_info);

  ////////////////////// Leptons /////////////////////
  if (debug) cout<<"INFO: Writing leptons..."<<endl;
  // pfcands, to be used in calculating isolation
  edm::Handle<pat::PackedCandidateCollection> pfcands;
  iEvent.getByLabel("packedPFCandidates", pfcands);

  // Event energy density, to be used in calculating isolation and JECs
  edm::Handle<double> rhoEventCentral_h;
  iEvent.getByLabel("fixedGridRhoFastjetCentralNeutral", rhoEventCentral_h);
  double rhoEventCentral = static_cast<double>(*rhoEventCentral_h);

  vCands sig_leps, veto_leps, sig_mus, veto_mus, sig_els, veto_els;
  vCands all_mus, all_els;
  edm::Handle<pat::MuonCollection> allmuons;
  iEvent.getByLabel("slimmedMuons", allmuons);
  sig_mus = writeMuons(allmuons, pfcands, vtx, veto_mus, all_mus, rhoEventCentral);
  edm::Handle<pat::ElectronCollection> allelectrons;
  iEvent.getByLabel("slimmedElectrons", allelectrons);
  sig_els = writeElectrons(allelectrons, pfcands, vtx, veto_els, all_els, rhoEventCentral);
  
 

  // Putting muons and electrons together
  sig_leps = sig_mus;
  sig_leps.insert(sig_leps.end(), sig_els.begin(), sig_els.end());
  writeLeptons(sig_leps);
  // if (baby.nleps()<1) return;
  veto_leps = veto_mus;
  veto_leps.insert(veto_leps.end(), veto_els.begin(), veto_els.end());

  ///////////////////////////////// Photons ////////////////////////////////
  if (debug) cout<<"INFO: Writing photons..."<<endl;
  vCands photons;
  edm::Handle<double> rhoEvent_h;
  iEvent.getByLabel( "fixedGridRhoFastjetAll", rhoEvent_h);
  edm::Handle<reco::BeamSpot> beamspot;
  iEvent.getByLabel("offlineBeamSpot", beamspot);
  edm::Handle<pat::PhotonCollection> allphotons;
  iEvent.getByLabel("slimmedPhotons", allphotons);
  edm::Handle<vector<reco::Conversion> > conversions;
  edm::InputTag converstionsTag("reducedEgamma","reducedConversions");
  iEvent.getByLabel(converstionsTag, conversions);
  photons = writePhotons(allphotons, allelectrons, conversions, beamspot, *rhoEvent_h);

  //////////////////////////// MET/JETs with JECs ///////////////////////////
  if (debug) cout<<"INFO: Applying JECs..."<<endl;
  edm::Handle<pat::JetCollection> alljets;
  iEvent.getByLabel(jets_label, alljets);
  edm::Handle<edm::View<reco::GenJet> > genjets;
  iEvent.getByLabel("slimmedGenJets", genjets) ;
  jetTool->getJetCorrections(genjets, alljets, *rhoEvent_h);

  /// MET
  if (debug) cout<<"INFO: Writing MET..."<<endl;
  edm::Handle<pat::METCollection> mets;
  iEvent.getByLabel(met_label, mets);
  edm::Handle<pat::METCollection> mets_nohf;
  iEvent.getByLabel(met_nohf_label, mets_nohf);
  writeMET(mets, mets_nohf);

  /// isolated tracks need to be after MET calculation and before jets cleaning
  if (debug) cout<<"INFO: Calculating track veto..."<<endl;
  vCands tks,ra4tks;
  if (eventTool->hasGoodPV(vtx)){
    // RA2/b track veto
    tks = lepTool->getIsoTracks(pfcands, baby.met(), baby.met_phi());
    baby.ntks() = tks.size();

    // RA4 track veto
    if(baby.leps_id().size()>0){    
      vector<float> isos;
      ra4tks = lepTool->getRA4IsoTracks(pfcands, baby.met(), baby.met_phi(),rhoEventCentral,isos,baby.leps_id().at(0));
      vector<float> tks_pt;
      vector<float> tks_eta;
      vector<float> tks_phi;
      vector<float> tks_d0;
      vector<float> tks_dz;
      vector<int> tks_pdg;
      vector<float> tks_miniso;
      vector<float> tks_mt2;
      vector<float> tks_mt;
      vector<bool> tks_tm;
     
      int nveto=0;

      for(unsigned i=0;i<ra4tks.size();i++){
        tks_pt.push_back(ra4tks.at(i)->pt());
        tks_eta.push_back(ra4tks.at(i)->eta());
        tks_phi.push_back(ra4tks.at(i)->phi());
	tks_d0.push_back( sqrt(pow(ra4tks.at(i)->vx()-vtx->at(0).x(),2) + pow(vtx->at(0).y()-ra4tks.at(i)->vy(),2)));
        tks_dz.push_back(ra4tks.at(i)->vz()-vtx->at(0).z());
        tks_pdg.push_back(ra4tks.at(i)->pdgId());
        tks_miniso.push_back(isos.at(i));
	tks_tm.push_back(false); //filled in writeMC
        tks_mt2.push_back(getMT2(baby.leps_pt().at(0),baby.leps_phi().at(0),tks_pt.back(),tks_phi.back(),baby.met(),baby.met_phi()));
        tks_mt.push_back(getMT(tks_pt.back(),tks_phi.back(),baby.met(),baby.met_phi()));
        if(fabs(tks_pdg.back())==211 && tks_pt.back()>15. && tks_miniso.back()<0.05 && tks_mt2.back()<100) nveto++;
        else if (fabs(tks_pdg.back())==13 && tks_pt.back()>10. && tks_miniso.back()<0.2 && tks_mt2.back()<100) nveto++;
        else if (fabs(tks_pdg.back())==11 && tks_pt.back()>10. && tks_miniso.back()<0.1 && tks_mt2.back()<100) nveto++;

	


      }
      baby.tks_pt() = tks_pt;
      baby.tks_eta() = tks_eta;
      baby.tks_phi() = tks_phi;
      baby.tks_pdg() = tks_pdg;
      baby.tks_miniso() = tks_miniso;
      baby.tks_mt2() = tks_mt2;
      baby.tks_mt() = tks_mt;
      baby.tks_tm() = tks_tm;
      baby.nveto()=nveto;

    }  
  } // if goodPV

  /// Jets
  if (debug) cout<<"INFO: Writing jets..."<<endl;
  vector<LVector> jets;
  vector<vector<LVector> > sys_jets;
  writeJets(alljets, genjets, sig_leps, veto_leps, photons, tks, jets, sys_jets);
 
  writeFatJets(jets);

  ////////////////////// mT, dphi /////////////////////
  // It requires previous storing of MET
  if (debug) cout<<"INFO: Calculating mT..."<<endl;
  if(sig_leps.size()>0){
    float wx = baby.met()*cos(baby.met_phi()) + sig_leps[0]->px();
    float wy = baby.met()*sin(baby.met_phi()) + sig_leps[0]->py();
    float wphi = atan2(wy, wx);
    baby.dphi_wlep() = deltaPhi(wphi, sig_leps[0]->phi());

    baby.mt() = getMT(baby.met(), baby.met_phi(),  sig_leps[0]->pt(), sig_leps[0]->phi());
    baby.mt_nohf() = getMT(baby.met_nohf(), baby.met_nohf_phi(),  sig_leps[0]->pt(), sig_leps[0]->phi());
  }   
 

  ///////////////////// Filters ///////////////////////
  // the HBHE noise filter needs to be recomputed in early 2015 data
  if (debug) cout<<"INFO: Writing filters..."<<endl;
  edm::Handle<bool> filter_hbhe;
  if(iEvent.getByLabel("HBHENoiseFilterResultProducer","HBHENoiseFilterResult",filter_hbhe)) { 
    if(filter_hbhe.isValid()) 
      baby.pass_hbhe() = *filter_hbhe;
    else
      baby.pass_hbhe() = true;

    iEvent.getByLabel("HBHENoiseFilterResultProducer","HBHEIsoNoiseFilterResult",filter_hbhe);
    if(filter_hbhe.isValid()) 
      baby.pass_hbheiso() = *filter_hbhe;
    else
      baby.pass_hbheiso() = true;
  }
  edm::Handle<edm::TriggerResults> filterBits;
  std::string processLabel = isData ? "RECO":"PAT"; // prompt reco runs in the "RECO" process
  iEvent.getByLabel(edm::InputTag("TriggerResults","",processLabel),filterBits);  
  // re-recoed data will have the process label "PAT" rather than "RECO";
  // if the attempt to find data with "RECO" process fails, try "PAT"
  if(!filterBits.isValid() && isData) 
    iEvent.getByLabel(edm::InputTag("TriggerResults", "", "PAT"),filterBits);  
  if(filterBits.isValid()){
    const edm::TriggerNames &fnames = iEvent.triggerNames(*filterBits);
    //this method uses baby.pass_jets(), so call only after writeJets()!!
    writeFilters(fnames, filterBits, vtx);
  }


  ////////////////// MC particles and Truth-matching //////////////////
  if (!isData) {
    if (debug) cout<<"INFO: Writing MC particles..."<<endl;
    edm::Handle<reco::GenParticleCollection> genParticles;
    iEvent.getByLabel("prunedGenParticles", genParticles);
    writeMC(genParticles, all_mus, all_els, photons,pfcands, rhoEventCentral,vtx,allmuons,allelectrons);
  }

  ////////////////// resolution-corrected MET /////////////////////////
  if (debug) cout<<"INFO: Calculating MET rebalancing..."<<endl;
  baby.jetmismeas() = false;
  if(doMetRebalancing && sig_leps.size()==1) {
    double temp_met = baby.met();
    double temp_met_phi = baby.met_phi();
    rebalancedMET(temp_met, temp_met_phi);
    baby.met_rebal() = temp_met;
    baby.mt_rebal() = getMT(temp_met, temp_met_phi, sig_leps[0]->pt(), sig_leps[0]->phi());
    if(baby.met_rebal()/baby.met()<0.2 && baby.mt_rebal()<150) baby.jetmismeas()=true;
  } else {
    // use default values for events that do not have exactly one lepton
    baby.mt_rebal() = baby.mt();
    baby.met_rebal() = baby.met();
  }

  ///////////////////// MC hard scatter info ///////////////////////
  if (debug) cout<<"INFO: Retrieving hard scatter info..."<<endl;
  edm::Handle<LHEEventProduct> lhe_info;
  baby.stitch() = true;
  if (!isData) {
    iEvent.getByLabel("externalLHEProducer", lhe_info);
    if(!lhe_info.isValid()) iEvent.getByLabel("source", lhe_info);
    writeGenInfo(lhe_info);
    if((outname.Contains("TTJets") && outname.Contains("Lept") && baby.ht_isr_me()>600)
       || (outname.Contains("DYJetsToLL_M-50_TuneCUETP8M")  && baby.ht_isr_me()>100))
      baby.stitch() = false;
  } // if it is not data
         
  //////////////// Weights and systematics //////////////////
  // w_btag calculated in writeJets
  // w_toppt and sys_isr calculated in writeMC
  edm::Handle<GenEventInfoProduct> gen_event_info;
  iEvent.getByLabel("generator", gen_event_info);
  writeWeights(sig_leps, gen_event_info, lhe_info);


  ////////////////// Filling the tree //////////////////
  baby.Fill();

  reportTime(iEvent);

}


/*
______                      _                     _ _   _             
| ___ \                    | |                   (_) | (_)            
| |_/ /_ __ __ _ _ __   ___| |__   __      ___ __ _| |_ _ _ __   __ _ 
| ___ \ '__/ _` | '_ \ / __| '_ \  \ \ /\ / / '__| | __| | '_ \ / _` |
| |_/ / | | (_| | | | | (__| | | |  \ V  V /| |  | | |_| | | | | (_| |
\____/|_|  \__,_|_| |_|\___|_| |_|   \_/\_/ |_|  |_|\__|_|_| |_|\__, |
                                                                 __/ |
                                                                |___/ 
*/

// Requires having called jetTool->getJetCorrections(alljets, rhoEvent_) beforehand
void bmaker_heller::writeMET(edm::Handle<pat::METCollection> mets, edm::Handle<pat::METCollection> mets_nohf){
  jetTool->getMETWithJEC(mets, baby.met(), baby.met_phi(), kSysLast);
  jetTool->getMETRaw(mets, baby.met_raw(), baby.met_raw_phi());
  baby.met_mini() = mets->at(0).pt();
  baby.met_mini_phi() = mets->at(0).phi();
  baby.met_calo() = mets->at(0).caloMETPt();
  baby.met_calo_phi() = mets->at(0).caloMETPhi();
  if(mets_nohf.isValid()){
    baby.met_nohf() = mets_nohf->at(0).pt();
    baby.met_nohf_phi() = mets_nohf->at(0).phi();
  }
  if(!isData){
    baby.met_tru() = mets->at(0).genMET()->pt();
    baby.met_tru_phi() = mets->at(0).genMET()->phi();
  }
  if(isnan(baby.met())) {
    cout<<"MET is NaN. Perhaps you are running on old miniAOD with a new release. Exiting"<<endl;
    exit(1);
  }
}

// Requires having called jetTool->getJetCorrections(alljets, rhoEvent_) beforehand
void bmaker_heller::writeJets(edm::Handle<pat::JetCollection> alljets, 
                             edm::Handle<edm::View <reco::GenJet> > genjets,
                             vCands &sig_leps, vCands &veto_leps, vCands &photons, vCands &tks,
                             vector<LVector> &jets, vector<vector<LVector> > &sys_jets){
  vCands jets_ra2;
  LVector jetsys_p4, jetsys_nob_p4;
  baby.njets() = 0; baby.nbl() = 0; baby.nbm() = 0;  baby.nbt() = 0;  
  baby.ht() = 0.; baby.ht_hlt() = 0.;
  baby.njets_ra2() = 0; baby.njets_clean() = 0; baby.nbm_ra2() = 0; baby.ht_ra2() = 0.; baby.ht_clean() = 0.; 
  baby.pass_jets() = true; baby.pass_jets_nohf() = true; baby.pass_jets_tight() = true; 
  baby.pass_jets_ra2() = true; baby.pass_jets_tight_ra2() = true; 
  
  baby.w_btag() = 1.;
 
  float mht_px(0.), mht_py(0.), mht_clean_px(0.), mht_clean_py(0.);
  for (size_t ijet(0); ijet < alljets->size(); ijet++) {
    const pat::Jet &jet = (*alljets)[ijet];
    if(fabs(jet.eta()) > 5) continue;

    LVector jetp4(jetTool->corrJet[ijet]);
    float csv(jet.bDiscriminator("pfCombinedInclusiveSecondaryVertexV2BJetTags"));
    bool isLep = jetTool->leptonInJet(jet, sig_leps);
    bool looseID = jetTool->idJet(jet, jetTool->kLoose);
    bool tightID = jetTool->idJet(jet, jetTool->kTight);
    bool goodPtEta = jetp4.pt() > jetTool->JetPtCut && fabs(jet.eta()) <= jetTool->JetEtaCut;

    //   RA4 Jet Quality filters
    //--------------------------------
    if(jetp4.pt() > jetTool->JetPtCut && !isLep) {
        if(goodPtEta && !looseID) baby.pass_jets_nohf() = false;
        if(!looseID) baby.pass_jets() = false;
        if(!tightID) baby.pass_jets_tight() = false;
    }

    //    RA4 Jet Variables
    //----------------------------
    if((looseID && goodPtEta) || isLep) {
      baby.jets_pt().push_back(jetp4.pt());
      baby.jets_eta().push_back(jet.eta());
      baby.jets_phi().push_back(jet.phi());
      baby.jets_m().push_back(jetp4.mass());
      baby.jets_islep().push_back(isLep);
      if(!isData && jetTool->genJetPt[ijet]>0.) baby.jets_pt_res().push_back(jetp4.pt()/jetTool->genJetPt[ijet]);
      else baby.jets_pt_res().push_back(-99999.);
      baby.jets_hflavor().push_back(jet.hadronFlavour());
      baby.jets_csv().push_back(csv);
      jets.push_back(jetp4);


      if(!isLep){
       
        jetsys_p4 += jet.p4();
        baby.njets()++;
        baby.ht() += jetp4.pt();
        if(csv > jetTool->CSVLoose)  baby.nbl()++;
        if(csv > jetTool->CSVMedium) baby.nbm()++;
        else jetsys_nob_p4 += jet.p4();
        if(csv > jetTool->CSVTight)  baby.nbt()++;
      }
    }

    //    HLT HT definition
    //----------------------------
    if(jetp4.pt() > jetTool->JetHLTPtCut && fabs(jet.eta()) <= jetTool->JetHLTEtaCut) baby.ht_hlt() += jetp4.pt();

    //    RA2 Jet Variables
    //----------------------------
    bool isLep_ra2 = jetTool->jetMatched(jet, veto_leps); // Uses RA2/b's loose matching, dpt/pt < 100%, dR < 0.4
    bool isPhoton = jetTool->jetMatched(jet, photons);    // Uses RA2/b's loose matching, dpt/pt < 100%, dR < 0.4
    bool isIsoTrack = jetTool->jetMatched(jet, tks);      // Uses RA2/b's loose matching, dpt/pt < 100%, dR < 0.4
    bool applyId_ra2 = !isLep_ra2 && !isPhoton && !isIsoTrack; // Only check ID if jet not matched
    bool goodJet_ra2 = (looseID || !applyId_ra2);
    bool tightJet_ra2 = (tightID || !applyId_ra2);

    if(jetp4.pt() > jetTool->JetPtCut) { // Check jet ID on 30 GeV jets
      if(!goodJet_ra2) baby.pass_jets_ra2() = false;
      if(!tightJet_ra2) baby.pass_jets_tight_ra2() = false;
    }

    if(goodPtEta && goodJet_ra2) {
      baby.njets_ra2()++;
      baby.ht_ra2() += jetp4.pt();
      jets_ra2.push_back(dynamic_cast<const reco::Candidate *>(&jet)); 
      if(csv > jetTool->CSVMedium) baby.nbm_ra2()++;
      if(!isLep_ra2 && !isPhoton) {
        baby.njets_clean()++;
        baby.ht_clean() += jetp4.pt();
      }
    }
    if(goodJet_ra2 && jetp4.pt() > jetTool->JetPtCut && fabs(jet.eta()) <= jetTool->JetMHTEtaCut){
      mht_px -= jet.px();
      mht_py -= jet.py();
      if(!isLep_ra2 && !isPhoton) {
        mht_clean_px -= jet.px();
        mht_clean_py -= jet.py();
      }
    }

    
  } // Loop over jets  

  if(!isData) baby.ht_tru() = jetTool->trueHT(genjets);

  baby.mht() = hypot(mht_px, mht_py);
  baby.mht_phi() = atan2(mht_py, mht_px);
  baby.mht_clean() = hypot(mht_clean_px, mht_clean_py);
  baby.mht_clean_phi() = atan2(mht_clean_py, mht_clean_px);
  baby.low_dphi() = jetTool->isLowDphi(jets_ra2, baby.mht_phi(), baby.dphi1(), baby.dphi2(), baby.dphi3(), baby.dphi4());

 
  
 

  // write deltaR between csvm jets
  jetTool->getDeltaRbb(baby.dr_bb(), jets, baby.jets_csv(), baby.jets_islep());

}

void bmaker_heller::writeFatJets(vector<LVector> &jets){
  jetTool->clusterFatJets(baby.nfjets(), baby.mj(),
                          baby.fjets_pt(), baby.fjets_eta(),
                          baby.fjets_phi(), baby.fjets_m(),
                          baby.fjets_nconst(),
                          baby.fjets_sumcsv(), baby.fjets_poscsv(),
                          baby.fjets_btags(), baby.jets_fjet_index(),
                          1.2, jets, baby.jets_csv());
  jetTool->clusterFatJets(baby.nfjets08(), baby.mj08(),
                          baby.fjets08_pt(), baby.fjets08_eta(),
                          baby.fjets08_phi(), baby.fjets08_m(),
                          baby.fjets08_nconst(),
                          baby.fjets08_sumcsv(), baby.fjets08_poscsv(),
                          baby.fjets08_btags(), baby.jets_fjet08_index(),
                          0.8, jets, baby.jets_csv());

}

vCands bmaker_heller::writeMuons(edm::Handle<pat::MuonCollection> muons, 
                                edm::Handle<pat::PackedCandidateCollection> pfcands, 
                                edm::Handle<reco::VertexCollection> vtx,
                                vCands &veto_mus, vCands &all_mus, double rhoEventCentral){
  vCands sig_mus; 
  veto_mus.clear(); all_mus.clear();
  baby.nmus() = 0; baby.nvmus() = 0;
  for (size_t ilep(0); ilep < muons->size(); ilep++) {
    const pat::Muon &lep = (*muons)[ilep];    
    if(!lepTool->isVetoMuon(lep, vtx, -99.)) continue; // Storing leptons that pass all veto cuts except for iso

    double lep_iso(lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&lep), 0.05, 0.2, 10., rhoEventCentral, false));
    double lep_reliso(lepTool->getRelIsolation(lep, rhoEventCentral));
    double dz(0.), d0(0.);
    lepTool->vertexMuon(lep, vtx, dz, d0); // Calculating dz and d0

    baby.mus_pt().push_back(lep.pt());
    baby.mus_eta().push_back(lep.eta());
    baby.mus_phi().push_back(lep.phi());
    baby.mus_dz().push_back(dz);
    baby.mus_d0().push_back(d0);
    baby.mus_charge().push_back(lep.charge());
    baby.mus_sigid().push_back(lepTool->idMuon(lep, vtx, lepTool->kMedium));
    baby.mus_tight().push_back(lepTool->idMuon(lep, vtx, lepTool->kTight));
    baby.mus_miniso().push_back(lep_iso);
    baby.mus_reliso().push_back(lep_reliso);
    baby.mus_tm().push_back(false);        // Filled in writeMC
    
    all_mus.push_back(dynamic_cast<const reco::Candidate *>(&lep)); // For truth-matching in writeMC

    if(lepTool->isVetoMuon(lep, vtx, lep_iso)) {
      baby.nvmus()++;
      veto_mus.push_back(dynamic_cast<const reco::Candidate *>(&lep));
    }
    if(lepTool->isSignalMuon(lep, vtx, lep_iso)) {
      baby.nmus()++;
      sig_mus.push_back(dynamic_cast<const reco::Candidate *>(&lep));
      baby.mus_sig().push_back(true); 
    } else baby.mus_sig().push_back(false); 
  } // Loop over muons
  
  return sig_mus;
}


vCands bmaker_heller::writeElectrons(edm::Handle<pat::ElectronCollection> electrons, 
                                    edm::Handle<pat::PackedCandidateCollection> pfcands, 
                                    edm::Handle<reco::VertexCollection> vtx,
                                    vCands &veto_els, vCands &all_els, double rhoEventCentral){
  vCands sig_els; 
  veto_els.clear(); all_els.clear();
  baby.nels() = 0; baby.nvels() = 0;
  for (size_t ilep(0); ilep < electrons->size(); ilep++) {
    const pat::Electron &lep = (*electrons)[ilep];    
    if(!lepTool->isVetoElectron(lep, vtx, -99.)) continue; // Storing leptons that pass all veto cuts except for iso

    double lep_iso(lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&lep), 0.05, 0.2, 10., rhoEventCentral, false));
    double lep_reliso(lepTool->getRelIsolation(lep, rhoEventCentral));
    double dz(0.), d0(0.);
    lepTool->vertexElectron(lep, vtx, dz, d0); // Calculating dz and d0

    baby.els_pt().push_back(lep.pt());
    baby.els_sceta().push_back(lep.superCluster()->position().eta());
    baby.els_eta().push_back(lep.eta());
    baby.els_phi().push_back(lep.phi());
    baby.els_dz().push_back(dz);
    baby.els_d0().push_back(d0);
    baby.els_charge().push_back(lep.charge());
    baby.els_sigid().push_back(lepTool->idElectron(lep, vtx, lepTool->kMedium));
    baby.els_ispf().push_back(lep.numberOfSourceCandidatePtrs()==2 && abs(lep.sourceCandidatePtr(1)->pdgId())==11);
    baby.els_tight().push_back(lepTool->idElectron(lep, vtx, lepTool->kTight));
    baby.els_miniso().push_back(lep_iso);
    baby.els_reliso().push_back(lep_reliso);
    baby.els_tm().push_back(false);       // Filled in writeMC
    all_els.push_back(dynamic_cast<const reco::Candidate *>(&lep)); // For truth-matching in writeMC

    if(lepTool->isVetoElectron(lep, vtx, lep_iso)){
      baby.nvels()++;
      veto_els.push_back(dynamic_cast<const reco::Candidate *>(&lep));
    }
    if(lepTool->isSignalElectron(lep, vtx, lep_iso)) {
      baby.nels()++;
      sig_els.push_back(dynamic_cast<const reco::Candidate *>(&lep));
      baby.els_sig().push_back(true); 
    } else baby.els_sig().push_back(false); 
  } // Loop over electrons

  return sig_els;
}

void bmaker_heller::writeLeptons(vCands &leptons){ 
  baby.nleps() = baby.nmus() + baby.nels();
  baby.nvleps() = baby.nvmus() + baby.nvels();
  sort(leptons.begin(), leptons.end(), greaterPt);
  for(size_t ind(0); ind < leptons.size(); ind++) {
    baby.leps_pt().push_back(leptons[ind]->pt());
    baby.leps_eta().push_back(leptons[ind]->eta());
    baby.leps_phi().push_back(leptons[ind]->phi());
    baby.leps_id().push_back(leptons[ind]->pdgId());
  } // Loop over leptons
}







vCands bmaker_heller::writePhotons(edm::Handle<pat::PhotonCollection> allphotons, 
                                  edm::Handle<std::vector<pat::Electron> > &electrons,
                                  edm::Handle<reco::ConversionCollection> &conversions,
                                  edm::Handle<reco::BeamSpot> &beamspot, double rho){
  vCands photons;
  baby.nph() = 0;

  for (size_t ind(0); ind < allphotons->size(); ind++) {
    const pat::Photon &photon = (*allphotons)[ind];    

    if(photon.pt() < 50) continue;
    if(!photonTool->idPhoton(photon, electrons, conversions, beamspot, rho)) continue;

    if(photon.pt() > photonTool->PhotonPtCut) baby.nph()++;
    baby.ph_pt().push_back(photon.pt());
    baby.ph_eta().push_back(photon.eta());
    baby.ph_phi().push_back(photon.phi());
    baby.ph_tm().push_back(false);          // Filled in writeMC
 

    photons.push_back(dynamic_cast<const reco::Candidate *>(&photon)); 
  } // Loop over allphotons

  return photons;
} // writePhotons

bool bmaker_heller::writeTriggers(const edm::TriggerNames &names, 
                                 edm::Handle<edm::TriggerResults> triggerBits, 
                                 edm::Handle<pat::PackedTriggerPrescales> triggerPrescales){
  bool keep(false);
  baby.trig().resize(trig_name.size(), false);
  baby.trig_prescale().resize(trig_name.size(), -1.);
  for (size_t itrig(0); itrig < triggerBits->size(); itrig++) {
    for(size_t itn(0); itn < trig_name.size(); itn++){
      if(names.triggerName(itrig).find(trig_name[itn])!=string::npos){
        baby.trig()[itn] = triggerBits->accept(itrig);
        baby.trig_prescale()[itn] = triggerPrescales->getPrescaleForIndex(itrig);
        if(baby.trig()[itn]) keep = true;
      }
    }
  }

  return keep;
}


// From https://twiki.cern.ch/twiki/bin/viewauth/CMS/MissingETOptionalFiltersRun2
void bmaker_heller::writeFilters(const edm::TriggerNames &fnames,
                                  edm::Handle<edm::TriggerResults> filterBits,
                                  edm::Handle<reco::VertexCollection> vtx){
  baby.pass_goodv() = true; baby.pass_cschalo() = true; baby.pass_eebadsc() = true;
  for (size_t i(0); i < filterBits->size(); ++i) {
    string name = fnames.triggerName(i);
    bool pass = static_cast<bool>(filterBits->accept(i));
    if (name=="Flag_goodVertices") baby.pass_goodv() = pass;
    //else if (name=="Flag_CSCTightHaloFilter") baby.pass_cschalo() = pass; // Requires reading it from txt file
    else if (name=="Flag_eeBadScFilter") baby.pass_eebadsc() = pass;
    //else if (name=="Flag_HBHENoiseFilter") baby.pass_hbhe() = pass; // Requires re-running in 2015
  }

  //baby.pass_goodv() &= eventTool->hasGoodPV(vtx); // We needed to re-run it for Run2015B
  baby.pass_cschalo() = eventTool->passBeamHalo(baby.run(), baby.event());

  baby.pass() = baby.pass_goodv() && baby.pass_eebadsc() && baby.pass_cschalo() && baby.pass_hbhe() && baby.pass_hbheiso() 
    && baby.pass_jets();
  baby.pass_ra2() = baby.pass_goodv() && baby.pass_eebadsc() && baby.pass_cschalo() && baby.pass_hbhe() && baby.pass_hbheiso()  
    && baby.pass_jets_ra2();
  baby.pass_nohf() = baby.pass_goodv() && baby.pass_eebadsc() && baby.pass_cschalo() && baby.pass_hbhe() && baby.pass_hbheiso()  
    && baby.pass_jets_nohf();

 
}

void bmaker_heller::writeVertices(edm::Handle<reco::VertexCollection> vtx,
                                 edm::Handle<std::vector< PileupSummaryInfo > >  pu_info){  
  baby.npv() = vtx->size();
  if(pu_info.isValid()){
    for(size_t bc(0); bc<pu_info->size(); ++bc){
      if(pu_info->at(bc).getBunchCrossing()==0){
        baby.ntrupv() = pu_info->at(bc).getPU_NumInteractions();
        baby.ntrupv_mean() = pu_info->at(bc).getTrueNumInteractions();
        break;
      }
    } // Loop over true PVs
  } // if pu_info is valid
}

void bmaker_heller::writeGenInfo(edm::Handle<LHEEventProduct> lhe_info){
  baby.nisr_me()=0; baby.ht_isr_me()=0.; 
  for ( unsigned int icount = 0 ; icount < (unsigned int)lhe_info->hepeup().NUP; icount++ ) {
    unsigned int pdgid = abs(lhe_info->hepeup().IDUP[icount]);
    int status = lhe_info->hepeup().ISTUP[icount];
    int mom1id = abs(lhe_info->hepeup().IDUP[lhe_info->hepeup().MOTHUP[icount].first-1]);
    int mom2id = abs(lhe_info->hepeup().IDUP[lhe_info->hepeup().MOTHUP[icount].second-1]);
    float px = (lhe_info->hepeup().PUP[icount])[0];
    float py = (lhe_info->hepeup().PUP[icount])[1];
    float pt = sqrt(px*px+py*py);

    if(status==1 && (pdgid<6 || pdgid==21) && mom1id!=6 && mom2id!=6 && mom1id!=24 && mom2id!=24 
       && mom1id!=23 && mom2id!=23 && mom1id!=25 && mom2id!=25) {
       baby.nisr_me()++;
       baby.ht_isr_me() += pt;
    }

  } // Loop over generator particles
  
  if(outname.Contains("T1tttt") || outname.Contains("T5ZZ") || outname.Contains("T5qqqqVV") ){ //Get mgluino and mlsp
    
    typedef std::vector<std::string>::const_iterator comments_const_iterator;
    
    comments_const_iterator c_begin = lhe_info->comments_begin();
    comments_const_iterator c_end = lhe_info->comments_end();
    
    TString model_params;
    for(comments_const_iterator cit=c_begin; cit!=c_end; ++cit) {
      size_t found = (*cit).find("model");
      if(found != std::string::npos)   {
	//    std::cout <<"BABYMAKER: "<< *cit <<"end"<< std::endl;  
        model_params = *cit;
      }
    }
    mcTool->getMassPoints(model_params,baby.mgluino(),baby.mlsp());
  }
} // writeGenInfo

void bmaker_heller::writeMC(edm::Handle<reco::GenParticleCollection> genParticles, 
			    vCands &all_mus, vCands &all_els, vCands &photons,  edm::Handle<pat::PackedCandidateCollection> pfcands, double rhoEventCentral,edm::Handle<reco::VertexCollection> vtx, edm::Handle<pat::MuonCollection> muons, edm::Handle<pat::ElectronCollection> electrons){
  LVector isr_p4;
  float metw_tru_x(0.), metw_tru_y(0.);
  float lep_tru_pt(0.), lep_tru_phi(0.);
  baby.ntruleps()=0; baby.ntrumus()=0; baby.ntruels()=0; baby.ntrutaush()=0; baby.ntrutausl()=0;
  baby.nleps_tm()=0;
  baby.fromGS()=false;
  baby.m_tt()=0;
  vector<float> top_pt;
  int topIndex=-1;
  int nlost=0;
  int antitopIndex=-1;
  //cout<<"starting event with nleps = "<<baby.nleps()<<endl;
  for (size_t imc(0); imc < genParticles->size(); imc++) {
    const reco::GenParticle &mc = (*genParticles)[imc];

    size_t id(abs(mc.pdgId())), momid(0);
    if(mc.mother()) momid = abs(mc.mother()->pdgId());
    bool lastTop(mcTool->isLast(mc,6));
    bool lastGluino(mcTool->isLast(mc,1000021));
    bool lastLSP(mcTool->isLast(mc,1000022));
    bool lastChi02(mcTool->isLast(mc,1000023));
    bool lastChi11(mcTool->isLast(mc,1000024));
    bool lastZ(mcTool->isLast(mc,23));
    bool lastW(mcTool->isLast(mc,24));
    bool lastGravitino(mcTool->isLast(mc,1000039));
    bool bFromTop(id==5 && momid==6);
    bool nuFromZ((id==12 || id==14 || id==16) && momid==23);
    bool eFromTopZ(id==11 && (momid==24 || momid==23));
    bool muFromTopZ(id==13 && (momid==24 || momid==23));
    bool tauFromTopZ(id==15 && (momid==24 || momid==23));
    bool fromWOrWTau(mcTool->fromWOrWTau(mc));
    bool chgPionFromTau(id==211 && momid==15 && fromWOrWTau);
    // bool laste(mcTool->isLast(mc,11));
    //bool lastmu(mcTool->isLast(mc,13));
    //bool lasttau(mcTool->isLast(mc,15));
    
    if(lastTop) mc.pdgId()>0 ? topIndex=imc : antitopIndex=imc;


   
    
    //////// Finding p4 of ME ISR system
    if((lastTop && outname.Contains("TTJets")) || (lastGluino && outname.Contains("SMS")) || 
       (lastZ && outname.Contains("DY"))) isr_p4 -= mc.p4();

    //////// Saving interesting true particles
    if(lastTop || lastGluino || lastLSP || lastChi02 || lastChi11 || lastGravitino || lastZ || lastW || bFromTop || eFromTopZ || muFromTopZ || tauFromTopZ || nuFromZ || fromWOrWTau) {
      baby.mc_id().push_back(mc.pdgId());
      baby.mc_pt().push_back(mc.pt());
      baby.mc_eta().push_back(mc.eta());
      baby.mc_phi().push_back(mc.phi());
      baby.mc_mass().push_back(mc.mass());
      baby.mc_mom().push_back(mc.mother()->pdgId());
    }
    if(lastTop && outname.Contains("TTJets")){
      top_pt.push_back(mc.pt());
    }
   
    
    //////// Counting true leptons
    if(muFromTopZ) baby.ntrumus()++;
    if(eFromTopZ)  baby.ntruels()++;
    if(tauFromTopZ){
      const reco::GenParticle *tauDaughter(0);
      if(mcTool->decaysTo(mc, 11, tauDaughter) || mcTool->decaysTo(mc, 13, tauDaughter)){
        baby.mc_id().push_back(tauDaughter->pdgId());
        baby.mc_pt().push_back(tauDaughter->pt());
        baby.mc_eta().push_back(tauDaughter->eta());
        baby.mc_phi().push_back(tauDaughter->phi());
        baby.mc_mom().push_back(tauDaughter->mother()->pdgId());
        baby.ntrutausl()++;
      } else baby.ntrutaush()++;
    }

    //cout<<"ntruels, ntrumus, ntrutausl, ntrutaush"<<baby.ntruels()<<", "<<baby.ntrumus()<<", "<<baby.ntrutausl()<<", "<<baby.ntrutaush()<<endl;
    
    
    //Find lost leptons (without any signal lepton)
    //use slightly looser tolerance
    float relptThres(2.), drThres(0.15); 
    //if(mcTool->fromWOrWTau(mc) && (laste||lastmu||lasttau|| (mcTool->fromTau(mc) && abs(mc.pdgId())==211))){
    if(eFromTopZ|| muFromTopZ || chgPionFromTau){
      //cout<<"New prompt lep, pdgid = "<<mc.pdgId()<<endl;
      //first, decide if this gen lepton is lost
      //by default, hadronic taus are considered lost
      double mindr(999.);
      int minind(-1);
      
      for(size_t ind(0); ind < baby.leps_pt().size(); ind++) {
	double dr = dR(baby.leps_phi()[ind],mc.phi(),baby.leps_eta()[ind],mc.eta());
	//	cout<<"mc lep eta phi = "<<mc.eta()<<" "<<mc.phi()<<", reco lep eta phi "<<baby.leps_eta()[ind]<<" "<<baby.leps_phi()[ind]<<", dr = "<<dr<<endl;
	double drelpt(fabs((baby.leps_pt()[ind] - mc.pt())/mc.pt()));
	if(dr > drThres || drelpt > relptThres) continue;
	if(dr < mindr){
	  mindr = dr;
	  minind = ind;
	}
      }
      


      if(minind<0){ // this lepton is lost	  
	nlost++;
	
	float miniso=-1;
	float dpt=-999.;

	float dz=-999.;
	float d0 =-999.;
	
	bool has_track=false;
	bool has_pat=false;
	bool vertex=false;
	bool id=false;
	bool sigid=false;
	bool from_tau= mcTool->fromTau(mc);
	int num_chg_siblings=0;
	
	
	

	// see if it even has a track, unless it is the tau itself
	double mindrtrk=999.;
	int minindtrk=-1;
	//	if(abs(mc.pdgId())!=15){
	for (size_t ipf(0); ipf< pfcands->size(); ipf++) {
	  const pat::PackedCandidate &pfc = (*pfcands)[ipf];
	  if(pfc.pdgId() != mc.pdgId() ) continue;
	  double dr = deltaR(pfc, mc);
	  double drelpt = (fabs((pfc.pt() - mc.pt())/mc.pt()));
	  if(dr > drThres || drelpt > relptThres) continue;
	  if(dr < mindrtrk){
	    mindrtrk = dr;
	    minindtrk=ipf;
	  }
	}
	  //	}
	
	//Has a track
	if(minindtrk>=0){
	  has_track=true;
	  const pat::PackedCandidate &track = (*pfcands)[minindtrk];
	  //if it has a track, use isolation of the track itself
	  if(abs(mc.pdgId())==11 || abs(mc.pdgId())==13) miniso = lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&track), 0.05, 0.2, 10., rhoEventCentral, false);	  
	  else miniso = lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&track), 0.05, 0.2, 10., rhoEventCentral, true);
	  
	  dz = fabs(track.vz()-vtx->at(0).z());
	  d0 = fabs(track.dxy()-vtx->at(0).x()*sin(track.phi())+vtx->at(0).y()*cos(track.phi()));
    
	  if(dz <= 0.5 && d0 <= 0.2) vertex=true;
	 
	  dpt = track.pt()-mc.pt();
	  
	}


	//Now look at pat objects
	double mindrpat=999.;
	int minindpat=-1;

	if(abs(mc.pdgId())==11){
	  for (size_t ielec(0); ielec < electrons->size(); ielec++) {
	    const pat::Electron &elec = (*electrons)[ielec];    
	    double dr = deltaR(elec, mc);
	    double drelpt = (fabs((elec.pt() - mc.pt())/mc.pt()));
	    if(dr > drThres || drelpt > relptThres) continue;
	    if(dr < mindrpat){
	      mindrpat = dr;
	      minindpat=ielec;
	      
	    }	
	  }

	  if(minindpat>=0){ //pat electron found
	    const pat::Electron &elecsel = (*electrons)[minindpat];
	    has_pat=true;
	    double elec_iso(lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&elecsel), 0.05, 0.2, 10., rhoEventCentral, false)); //need to recalculate in case there is no track
	    id = lepTool->isVetoElectron(elecsel, vtx, elec_iso);
	    sigid = lepTool->isSignalElectron(elecsel, vtx, elec_iso);
	    //Only use pat values if track is not found
	    if(dpt<-900.) dpt = elecsel.pt()-mc.pt();
	    if(miniso<0) miniso = elec_iso;
	  }
	}

	else if(abs(mc.pdgId())==13){
	  for (size_t imu(0); imu < muons->size(); imu++) {
	    const pat::Muon &mu = (*muons)[imu];    
	    double dr = deltaR(mu, mc);
	    double drelpt = (fabs((mu.pt() - mc.pt())/mc.pt()));
	    if(dr > drThres || drelpt > relptThres) continue;
	    if(dr < mindrpat){
	      mindrpat = dr;
	      minindpat=imu;
	      
	    }	
	  }

	  if(minindpat>=0){
	    has_pat=true;
	    const pat::Muon &musel = (*muons)[minindpat];    
	    double mu_iso(lepTool->getPFIsolation(pfcands, dynamic_cast<const reco::Candidate *>(&musel), 0.05, 0.2, 10., rhoEventCentral, false)); //need to recalculate in case there is no track
	    id = lepTool->isVetoMuon(musel, vtx, mu_iso);
	    sigid = lepTool->isSignalMuon(musel, vtx, mu_iso);
	    //Only use pat values if track is not found
	    if(dpt<-900.) dpt = musel.pt()-mc.pt();
	    if(miniso<0) miniso = mu_iso;
	  }
	}

	//tau specific
	if(from_tau && abs(mc.pdgId())==211){
	  const reco::GenParticle *mcMom;
	  mcTool->mom(mc, mcMom);
	  num_chg_siblings = mcTool->numChargeDaughters(*mcMom)-1; 
	}


	/*	else if(abs(mc.pdgId())==15){
	  num_chg_daughters = mcTool->numChargeDaughters(mc);

	  if(mcTool->hasDaughter(mc, 11))
	    daughter=11;
	  else if(mcTool->hasDaughter(mc, 13))
	    daughter=13;
	  else
	    daughter=211;


	    }*/

	baby.lost_pdg().push_back(mc.pdgId());
	baby.lost_pt().push_back(mc.pt());
	baby.lost_dpt().push_back(dpt);
	baby.lost_d0().push_back(d0);
	baby.lost_dz().push_back(dz);
	baby.lost_eta().push_back(mc.eta());
	baby.lost_phi().push_back(mc.phi());
	baby.lost_id().push_back(id);
	baby.lost_sigid().push_back(sigid);
	baby.lost_from_tau().push_back(from_tau);
	baby.lost_vertex().push_back(vertex);
	baby.lost_has_trk().push_back(has_track);
	baby.lost_has_pat().push_back(has_pat);
	baby.lost_miniso().push_back(miniso);
	baby.lost_num_chg_siblings().push_back(num_chg_siblings);
      }
    }
    baby.nlost() = nlost;


    
    //////// Finding truth-matched leptons
    const float relptThreshold(0.3), drThreshold(0.1);    
    if(id==11 && mcTool->fromWOrWTau(mc)){
      double mindr(999.);
      int minind(-1);
      for(size_t ind(0); ind < all_els.size(); ind++) {
        double dr(deltaR(mc, *(all_els[ind])));
        double drelpt(fabs((all_els[ind]->pt() - mc.pt())/mc.pt()));
        if(dr > drThreshold || drelpt > relptThreshold) continue;
        if(dr < mindr){
          mindr = dr;
          minind = ind;
        }
      } // Loop over all_els
      
      if(minind >= 0) {
        baby.els_tm()[minind] = true;
        if(baby.els_sig()[minind]) baby.nleps_tm()++;
      }
      if(lep_tru_pt < mc.pt()){
        lep_tru_pt = mc.pt();
        lep_tru_phi = mc.phi();
      } // Lepton pt to find mt_tru
    } // If it is an electron
    if(id==13 && mcTool->fromWOrWTau(mc)){
      double mindr(999.);
      int minind(-1);
      for(size_t ind(0); ind < all_mus.size(); ind++) {
        double dr(deltaR(mc, *(all_mus[ind])));
        double drelpt(fabs((all_mus[ind]->pt() - mc.pt())/mc.pt()));
        if(dr > drThreshold || drelpt > relptThreshold) continue;
        if(dr < mindr){
          mindr = dr;
          minind = ind;
        }
      } // Loop over all_mus
      if(minind >= 0) {
        baby.mus_tm()[minind] = true;
        if(baby.mus_sig()[minind]) baby.nleps_tm()++;
      }
      if(lep_tru_pt < mc.pt()){
        lep_tru_pt = mc.pt();
        lep_tru_phi = mc.phi();
      } // Lepton pt to find mt_tru
    } // If it is a muon
    
    if((id==13||id==11) && mcTool->fromWOrWTau(mc)){
      double mindr(999.);
      int minind(-1);
      for(size_t ind(0); ind < baby.tks_pt().size(); ind++) {
        double dr = dR(baby.tks_phi()[ind],mc.phi(),baby.tks_eta()[ind],mc.eta());
        double drelpt(fabs((baby.tks_pt()[ind] - mc.pt())/mc.pt()));
        if(dr > drThreshold || drelpt > relptThres) continue;
        if(dr < mindr){
          mindr = dr;
          minind = ind;
        }
      } // Loop over tks
      if(minind >= 0) {
        baby.tks_tm()[minind] = true;
      }
     
    } // If it is a lepton
    

    
    if(id==22){
      double mindr(999.);
      int minind(-1);
      for(size_t ind(0); ind < photons.size(); ind++) {
        double dr(deltaR(mc, *(photons[ind])));
        double drelpt(fabs((photons[ind]->pt() - mc.pt())/mc.pt()));
        if(dr > drThreshold || drelpt > relptThreshold) continue;
        if(dr < mindr){
          mindr = dr;
          minind = ind;
        }
      } // Loop over photons
      if(minind >= 0) baby.ph_tm()[minind] = true;
    } // If it is a photon

    //////// Finding true MET
    if((id==12 || id==14 || id==16 || id==18 || id==1000012 || id==1000014 || id==1000016
        || id==1000022 || id==1000023 || id==1000025 || id==1000035 || id==1000039) &&
       id != momid){ // neutrinos decay to themselves
      if(mcTool->fromWOrWTau(mc)) {
        metw_tru_x += mc.px();
        metw_tru_y += mc.py();
      }
    } // If undetected neutral particle

    // don't need to check for gluon splitting if flag is already set
    if(!baby.fromGS()) baby.fromGS()|=mcTool->isFromGSP(dynamic_cast<const reco::Candidate*>(&mc));
  } // Loop over genParticles
  // calculate invariant mass of ttbar pair
  if(topIndex>=0 && antitopIndex>=0) {
    reco::Candidate::LorentzVector topP4 = genParticles->at(topIndex).p4();
    reco::Candidate::LorentzVector antitopP4 = genParticles->at(antitopIndex).p4();
    reco::Candidate::LorentzVector ttbarP4 = topP4+antitopP4;
    baby.m_tt()=ttbarP4.mass();
  }

  baby.ntruleps() = baby.ntrumus()+baby.ntruels()+baby.ntrutaush()+baby.ntrutausl();
  baby.isr_tru_pt() = isr_p4.pt();
  baby.isr_tru_eta() = isr_p4.eta();
  baby.isr_tru_phi() = isr_p4.phi();
  
  

  if(outname.Contains("TTJets") && top_pt.size() == 2) baby.w_toppt() = weightTool->topPtWeight(top_pt.at(0),top_pt.at(1));
  else baby.w_toppt() = 1.;

  baby.met_tru_nuw() = hypot(metw_tru_x, metw_tru_y);
  baby.met_tru_nuw_phi() = atan2(metw_tru_y, metw_tru_x);

  baby.mt_tru()     = getMT(baby.met_tru(),     baby.met_tru_phi(),     lep_tru_pt, lep_tru_phi);
  baby.mt_tru_nuw() = getMT(baby.met_tru_nuw(), baby.met_tru_nuw_phi(), lep_tru_pt, lep_tru_phi);

} // writeMC

// Finds the jet that minimizes the MET when a variation is performed
void bmaker_heller::rebalancedMET( double& minMET, double& minMETPhi)
{
  for(unsigned int iJet=0; iJet<baby.jets_pt().size(); iJet++) {
    // calculate best rescaling factor for this jet
    double rescalingFactor=calculateRescalingFactor(iJet);
    double newMETPhi=0;
    double newMET=calculateRebalancedMET(iJet, rescalingFactor, newMETPhi);
    if(newMET<minMET) {
      minMET=newMET;
      minMETPhi=newMETPhi;
    }
  }
}

// calculate a rebalancing of the jet momentum that minimizes MET
double bmaker_heller::calculateRescalingFactor(unsigned int jetIdx)
{
  
  // don't allow jet pt to be scaled by more than this factor
  const double scaleCutoff=1;
  
  TVector3 jet, metVector;
  jet.SetPtEtaPhi(baby.jets_pt().at(jetIdx), baby.jets_eta().at(jetIdx), baby.jets_phi().at(jetIdx));
  metVector.SetPtEtaPhi(baby.met(), 0, baby.met_phi());
  
  double denominator = -jet.Px()*jet.Px()-jet.Py()*jet.Py();
  double numerator = jet.Px()*metVector.Px()+jet.Py()*metVector.Py();
  
  double rescalingFactor=1e6;
  if(denominator!=0) rescalingFactor = numerator/denominator;
  if(fabs(rescalingFactor)>scaleCutoff) rescalingFactor=scaleCutoff*rescalingFactor/fabs(rescalingFactor);
  // the resolution tail is on the _low_ side, not the high side
  // so we always need to subtract pT
  if(rescalingFactor>0) rescalingFactor=0;
  
  return rescalingFactor;
}

double bmaker_heller::calculateRebalancedMET(unsigned int jetIdx, double mu, double& METPhi)
{
  TVector3 jet, metVector;
  jet.SetPtEtaPhi(baby.jets_pt().at(jetIdx), baby.jets_eta().at(jetIdx), baby.jets_phi().at(jetIdx));
  metVector.SetPtEtaPhi(baby.met(), 0, baby.met_phi());
 
  double sumPx = metVector.Px()+mu*jet.Px();
  double sumPy = metVector.Py()+mu*jet.Py();

  METPhi=atan(sumPy/sumPx);

  return sqrt(sumPx*sumPx+sumPy*sumPy);
}

void bmaker_heller::writeWeights(const vCands &sig_leps, edm::Handle<GenEventInfoProduct> &gen_event_info, 
                                edm::Handle<LHEEventProduct> &lhe_info){
  if (debug) cout<<"INFO: Filling weights..."<<endl;

  // Initializing weights
  if(isData) {
    baby.eff_trig() = baby.w_btag() = baby.w_pu() = baby.w_lep() = baby.w_fs_lep() = baby.w_toppt() = 1.;
    baby.eff_jetid() = baby.w_lumi() = baby.weight() = 1.;
    return;
  }

  // Luminosity weight
  const float luminosity = 1000., fneg(xsec::fractionNegWeights(outname));
  baby.w_lumi() = xsec*luminosity / (static_cast<double>(nevents_sample)*(1-2*fneg));
  if (gen_event_info->weight() < 0) baby.w_lumi() *= -1.;
  
  // Pile-up weight
  baby.w_pu() = weightTool->pileupWeight(baby.ntrupv_mean());

  // Lepton SFs
  double sf  = lepTool->getScaleFactor(sig_leps);
  baby.w_lep() = sf;
  

  // Lepton SFs in FastSim
  
  if(isFastSim){ 
    double sf_fs  = lepTool->getScaleFactorFs(sig_leps, baby.npv());
    baby.w_fs_lep() = sf_fs;
  
  } else baby.w_fs_lep() = 1.;

  // VVVL trigger efficiency
  vector<float> dumby(0.);
  baby.eff_trig() = weightTool->triggerEfficiency(baby.nmus(), baby.nels(), dumby);
  
  // In FastSim the JetID is broken, so we just apply 0.99 +- 0.01
  if(isFastSim) baby.eff_jetid() = 0.99;
  else baby.eff_jetid() = 1.;
  ////////////  Total weight  ////////////
  // w_btag calculated in writeJets
  // w_toppt and sys_isr calculated in writeMC
  baby.weight() = baby.w_lumi() * baby.w_lep() * baby.w_fs_lep() * baby.w_toppt() * baby.w_btag() 
    * baby.eff_trig() * baby.eff_jetid();

 
 


}

/*
 _____                 _                   _                 
/  __ \               | |                 | |                
| /  \/ ___  _ __  ___| |_ _ __ _   _  ___| |_ ___  _ __ ___ 
| |    / _ \| '_ \/ __| __| '__| | | |/ __| __/ _ \| '__/ __|
| \__/\ (_) | | | \__ \ |_| |  | |_| | (__| || (_) | |  \__ \
 \____/\___/|_| |_|___/\__|_|   \__,_|\___|\__\___/|_|  |___/
*/

bmaker_heller::bmaker_heller(const edm::ParameterSet& iConfig):
  outname(TString(iConfig.getParameter<string>("outputFile"))),
  inputfiles(iConfig.getParameter<vector<string> >("inputFiles")),
  jsonfile(iConfig.getParameter<string>("json")),
  condor_subtime(iConfig.getParameter<string>("condor_subtime")),
  jec_label(iConfig.getParameter<string>("jec")),
  met_label(iConfig.getParameter<edm::InputTag>("met")),
  met_nohf_label(iConfig.getParameter<edm::InputTag>("met_nohf")),
  jets_label(iConfig.getParameter<edm::InputTag>("jets")),
  nevents_sample(iConfig.getParameter<unsigned int>("nEventsSample")),
  nevents(0),
  doMetRebalancing(iConfig.getParameter<bool>("doMetRebalancing")),
  addBTagWeights(iConfig.getParameter<bool>("addBTagWeights")),
  isFastSim(iConfig.getParameter<bool>("isFastSim")),
  doSystematics(iConfig.getParameter<bool>("doSystematics")),
  debug(iConfig.getParameter<bool>("debugMode"))
{
  time(&startTime);

  lepTool    = new lepton_tools();
  jetTool    = new jet_met_tools(jec_label, doSystematics, isFastSim);
  photonTool = new photon_tools();
  mcTool     = new mc_tools();
  weightTool = new weight_tools();
  eventTool  = new event_tools(outname);


  outfile = new TFile(outname, "recreate");
  outfile->cd();
  baby.tree_.SetDirectory(outfile);

  xsec = xsec::crossSection(outname);
  if(xsec<=0) {
    cout<<"BABYMAKER: Cross section not found, aborting"<<endl<<endl;
    exit(1);
  }

  trig_name = vector<TString>();
  if(outname.Contains("Run201")){
    trig_name.push_back("HLT_PFHT350_PFMET100_");                               // 0 
    trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT350_PFMET50_v");                  // 1 
    trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT600_v");                          // 2
    trig_name.push_back("HLT_Mu15_IsoVVVL_BTagCSV0p72_PFHT400_v");              // 3
    trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT350_v");                          // 4 
    trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT350_PFMET50_v");                 // 5 
    trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT600_v");                         // 6
    trig_name.push_back("HLT_Ele15_IsoVVVL_BTagCSV0p72_PFHT400_v");             // 7
    trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT350_v");                         // 8 
    trig_name.push_back("HLT_DoubleMu8_Mass8_PFHT300_v");                       // 9
    trig_name.push_back("HLT_DoubleEle8_CaloIdM_TrackIdM_Mass8_PFHT300_v");     // 10
    trig_name.push_back("HLT_PFHT475_v");                                       // 11
    trig_name.push_back("HLT_PFHT800_v");                                       // 12
    trig_name.push_back("HLT_PFMET120_JetIdCleaned_Mu5_v");                     // 13
    trig_name.push_back("HLT_PFMET170_JetIdCleaned_v");                         // 14
    trig_name.push_back("HLT_DoubleIsoMu17_eta2p1_v");                          // 15
    trig_name.push_back("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v");               // 16
    trig_name.push_back("HLT_IsoMu20_v");                                       // 17
    trig_name.push_back("HLT_IsoMu18_v");                                       // 18
    trig_name.push_back("HLT_IsoMu24_eta2p1_v");                                // 19
    trig_name.push_back("HLT_IsoMu27_v");                                       // 20
    trig_name.push_back("HLT_Mu50_v");                                          // 21
    trig_name.push_back("HLT_Ele27_eta2p1_WPLoose_Gsf_v");                      // 22
    trig_name.push_back("HLT_Ele23_WPLoose_Gsf_v");                             // 23
    trig_name.push_back("HLT_Ele105_CaloIdVT_GsfTrkIdT_v");                     // 24
    trig_name.push_back("HLT_DoubleEle33_CaloIdL_GsfTrkIdVL_v");                // 25
    trig_name.push_back("HLT_DoubleEle24_22_eta2p1_WPLoose_Gsf_v");             // 26
    trig_name.push_back("HLT_Photon90_CaloIdL_PFHT500_v");                      // 27
    trig_name.push_back("HLT_PFMETNoMu90_JetIdCleaned_PFMHTNoMu90_IDTight_v");  // 28
  } else {
    trig_name.push_back("HLT_PFHT350_PFMET120_NoiseCleaned_v");                 // 0 
    trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT400_PFMET70_v");                  // 1 
    trig_name.push_back("HLT_Mu15_IsoVVVL_PFHT600_v");                          // 2
    trig_name.push_back("HLT_Mu15_IsoVVVL_BTagCSV07_PFHT400_v");                // 3
    trig_name.push_back("HLT_Mu15_PFHT300_v");                                  // 4 
    trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT400_PFMET70_v");                 // 5 
    trig_name.push_back("HLT_Ele15_IsoVVVL_PFHT600_v");                         // 6
    trig_name.push_back("HLT_Ele15_IsoVVVL_BTagtop8CSV07_PFHT400_v");           // 7
    trig_name.push_back("HLT_Ele15_PFHT300_v");                                 // 8 
    trig_name.push_back("HLT_DoubleMu8_Mass8_PFHT300_v");                       // 9
    trig_name.push_back("HLT_DoubleEle8_CaloIdM_TrackIdM_Mass8_PFHT300_v");     // 10
    trig_name.push_back("HLT_PFHT350_v");                                       // 11
    trig_name.push_back("HLT_PFHT900_v");                                       // 12
    trig_name.push_back("HLT_PFMET120_NoiseCleaned_Mu5_v");                     // 13
    trig_name.push_back("HLT_PFMET170_NoiseCleaned_v");                         // 14
    trig_name.push_back("HLT_DoubleIsoMu17_eta2p1_v");                          // 15
    trig_name.push_back("HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v");               // 16
    trig_name.push_back("HLT_IsoMu20_v");                                       // 17
    trig_name.push_back("HLT_IsoMu17_eta2p1_v");                                // 18
    trig_name.push_back("HLT_IsoMu24_eta2p1_v");                                // 19
    trig_name.push_back("HLT_IsoMu27_v");                                       // 20
    trig_name.push_back("HLT_Mu50_v");                                          // 21
    trig_name.push_back("HLT_Ele27_eta2p1_WP75_Gsf_v");                         // 22
    trig_name.push_back("HLT_Ele22_eta2p1_WP75_Gsf_v");                         // 23
    trig_name.push_back("HLT_Ele105_CaloIdVT_GsfTrkIdT_v");                     // 24
    trig_name.push_back("HLT_DoubleEle33_CaloIdL_GsfTrkIdVL_v");                // 25
    trig_name.push_back("HLT_DoubleEle24_22_eta2p1_WP75_Gsf_v");                // 26
    trig_name.push_back("HLT_Photon90_CaloIdL_PFHT500_v");                      // 27
    trig_name.push_back("HLT_PFMETNoMu90_JetIdCleaned_PFMHTNoMu90_IDTight_v");  // 28
  }

}


bmaker_heller::~bmaker_heller(){
  outfile->cd();
  baby.tree_.SetDirectory(outfile);
  baby.Write();

  string commit_s = execute("git rev-parse HEAD");
  while(!commit_s.empty() && commit_s.at(commit_s.length()-1) == '\n') commit_s.erase(commit_s.length()-1);
  TString commit = commit_s;
  TString type = baby.Type();
  TString root_version = gROOT->GetVersion();
  TString root_tutorial_dir = gROOT->GetTutorialsDir();
  TString user(getenv("ORIGIN_USER"));
  if (user=="") user = getenv("USER");
  TString cmssw(getenv("CMSSW_BASE"));
  time_t curTime;
  time(&curTime);
  char time_c[100];
  struct tm * timeinfo = localtime(&curTime);
  strftime(time_c,100,"%Y-%m-%d %H:%M:%S",timeinfo);
  TString date(time_c);
  int seconds(floor(difftime(curTime,startTime)+0.5));

  vector<TString> sys_names;
  sys_names.resize(kSysLast,"");
  sys_names[kSysJER] = "jer";
  sys_names[kSysJECUp] = "jec_up";
  sys_names[kSysJECDn] = "jec_dn";

  TTree treeglobal("treeglobal", "treeglobal");
  treeglobal.Branch("nev_sample", &nevents_sample);
  treeglobal.Branch("nev_file", &nevents);
  treeglobal.Branch("runtime_seconds", &seconds);
  treeglobal.Branch("git_commit", &commit);
  // treeglobal.Branch("model", &model);
  treeglobal.Branch("baby_type", &type);
  treeglobal.Branch("root_version", &root_version);
  treeglobal.Branch("root_tutorial_dir", &root_tutorial_dir);
  treeglobal.Branch("trig_names", &trig_name);
  treeglobal.Branch("sys_names", &sys_names);
  treeglobal.Branch("xsec", &xsec);
  treeglobal.Branch("user", &user);
  treeglobal.Branch("cmssw", &cmssw);
  treeglobal.Branch("jec", &jec_label);
  treeglobal.Branch("json", &jsonfile);
  treeglobal.Branch("date", &date);
  treeglobal.Branch("inputfiles", &inputfiles);
  treeglobal.Branch("condor_subtime", &condor_subtime);
  treeglobal.Fill();
  treeglobal.SetDirectory(outfile);
  treeglobal.Write();
  
  outfile->Close();

  int minutes((seconds/60)%60), hours(seconds/3600);
  TString runtime("");
  if(hours<10) runtime += "0";
  runtime += hours; runtime += ":";
  if(minutes<10) runtime += "0";
  runtime += minutes; runtime += ":";
  if((seconds%60)<10) runtime += "0";
  runtime += seconds%60; 
  float hertz(nevents); hertz /= seconds;
  cout<<endl<<"BABYMAKER: Written "<<nevents<<" events in "<<outname<<". It took "<<seconds<<" seconds to run ("<<runtime<<"), "
      <<roundNumber(hertz,1)<<" Hz, "<<roundNumber(1000,2,hertz)<<" ms per event"<<endl<<endl;
  cout<<"BABYMAKER: *********** List of input files ***********"<<endl;
  for(size_t ifile(0); ifile < inputfiles.size(); ifile++)
    cout<<"BABYMAKER: "<<inputfiles[ifile].c_str()<<endl;
  cout<<endl;

  delete outfile;

  delete lepTool;
  delete photonTool;
  delete jetTool;
  delete mcTool;
  delete weightTool;
}

void bmaker_heller::reportTime(const edm::Event& iEvent){
  // Time reporting
  if(nevents==1) {
    time_t curTime;
    time(&curTime);
    cout<<endl<<"BABYMAKER: Took "<<roundNumber(difftime(curTime,startTime),1)<<" seconds for set up and run first event"
        <<endl<<endl;
    time(&startTime);
  }
  if(debug || (nevents<100&&nevents%10==0) || (nevents<1000&&nevents%100==0) 
     || (nevents<10000&&nevents%1000==0) || nevents%10000==0) {
    time_t curTime;
    time(&curTime);
    float seconds(difftime(curTime,startTime));
    cout<<"BABYMAKER: Run "<<iEvent.id().run()<<", Event "<< setw(8)<<iEvent.id().event()
        <<", LumiSection "<< setw(5)<< iEvent.luminosityBlock()
        <<". Ran "<<setw(7)<<nevents<<" events in "<<setw(7)<<seconds<<" seconds -> "
        <<setw(5)<<roundNumber(nevents-1,1,seconds)<<" Hz, "
        <<setw(5)<<roundNumber(seconds*1000,2,nevents-1)<<" ms per event"<<endl;
  }
}

// ------------ method called once each job just before starting event loop  ------------
void bmaker_heller::beginJob() {
}

// ------------ method called once each job just after ending the event loop  ------------
void bmaker_heller::endJob() {
}

// ------------ method called when starting to processes a run  ------------
/*
void 
bmaker_heller::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a run  ------------
/*
void 
bmaker_heller::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when starting to processes a luminosity block  ------------
/*
void 
bmaker_heller::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method called when ending the processing of a luminosity block  ------------
/*
void 
bmaker_heller::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
bmaker_heller::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(bmaker_heller);