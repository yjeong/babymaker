// Common utilities

#ifndef H_UTILITIES
#define H_UTILITIES

#include <vector>
#include "DataFormats/PatCandidates/interface/Electron.h"
#include <fastjet/PseudoJet.hh>


typedef std::vector<const reco::Candidate*> vCands;

bool greaterPt(const reco::Candidate *a, const reco::Candidate *b);
bool greaterM(const fastjet::PseudoJet &a, const fastjet::PseudoJet &b);

#endif
