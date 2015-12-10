// change_filters: Reads in txt file and filters events accordingly

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <set>

#include "TFile.h"
#include "TString.h"
#include "TTree.h"

#include "utilities.hh"

using namespace std;

map<int, set<long> > getFilterList(string filter);
void change_filter_one(TString indir, TString name, TString outdir, map<int, set<long> > filter_list, TString filter_name);

int main(int argc, char *argv[]){
  time_t begtime, endtime;
  time(&begtime);

  TString filter(""), infolder("."), sample("*.root"), outfolder(""), filter_var("");
  int c(0);
  while((c=getopt(argc, argv, "f:i:s:o:v:"))!=-1){
    switch(c){
    case 'f':
      filter=optarg;
      break;
    case 'i':
      infolder=optarg;
      break;
    case 's':
      sample=optarg;
      break;
    case 'o':
      outfolder=optarg;
      break;
    case 'v':
      filter_var=optarg;
      break;
    default:
      break;
    }
  }

  if(filter=="" || filter_var==""){
    cout<<endl<<"Specify filter file and filter variable: "
        <<"./run/change_filter.exe -f <filter_file> -i <infolder>=. -s <sample>=\"*.root\" -o <outfolder>=<infolder> -v <filter_var>"<<endl<<endl;
    return 1;
  }
    
  if(outfolder=="") outfolder=infolder;
  if(!infolder.EndsWith("/")) infolder.Append("/");
  if(!outfolder.EndsWith("/")) outfolder.Append("/");

  map<int, set<long> > event_list = getFilterList(filter.Data());

  vector<TString> files = dirlist(infolder,sample);
  for(unsigned int i=0; i<files.size(); i++){
    if(i%100==0) cout<<"[Change Filter] File #"<<i+1<<"/"<<files.size()<<endl;
    change_filter_one(infolder, files[i], outfolder, event_list, filter_var);
  }

  time(&endtime);
  cout<<endl<<"Took "<<difftime(endtime, begtime)<<" seconds"<<endl<<endl;

}

map<int, set<long> > getFilterList(string filter){
  map<int, set<long> > event_list;

  ifstream infile(filter);
  string line;
  TString line_temp;

  while(getline(infile, line)){
    line_temp = line;
    line_temp.ReplaceAll(":"," ");
    istringstream iss(line_temp.Data());
    unsigned int irun, ils;
    long ievent;
    iss >> irun >> ils >> ievent;

    event_list[irun].insert(ievent);
  }
  infile.close();

  return event_list;
}


void change_filter_one(TString indir, TString name, TString outdir, map<int, set<long> > filter_list, TString filter_name){

  //Set up old file/tree
  TFile *oldfile = new TFile(indir+name);
  TTree* oldtree = static_cast<TTree*>(oldfile->Get("tree"));
  TTree* oldtreeglobal = static_cast<TTree*>(oldfile->Get("treeglobal"));  

  int run = 0;
  Long64_t event = 0;
  bool pass = false, pass_goodv = false, pass_eebadsc = false, pass_hbhe = false, pass_hbheiso = false, pass_jets = false;
  bool new_filter = false;

  //Set branch addresses
  oldtree->SetBranchAddress("run",&run);
  oldtree->SetBranchAddress("event",&event);
  oldtree->SetBranchAddress("pass",&pass);
  oldtree->SetBranchAddress(filter_name,&new_filter);
  oldtree->SetBranchAddress("pass_goodv",&pass_goodv);
  oldtree->SetBranchAddress("pass_eebadsc",&pass_eebadsc);
  //  oldtree->SetBranchAddress("pass_cschalo",&pass_cschalo);
  oldtree->SetBranchAddress("pass_hbhe",&pass_hbhe);
  oldtree->SetBranchAddress("pass_hbheiso",&pass_hbheiso);
  oldtree->SetBranchAddress("pass_jets",&pass_jets);

  //Set up new tree
  name.ReplaceAll(".root","_refilter.root");
  TFile* newfile = new TFile(outdir+name,"recreate");
  TTree* newtree = oldtree->CloneTree(0);
  TTree* newtreeglobal = oldtreeglobal->CloneTree();

  long nentries = oldtree->GetEntries();

  for(long i=0; i<nentries; i++){
    oldtree->GetEntry(i);

    map<int, set<long> >::iterator imap = filter_list.find(run);
    if(imap==filter_list.end()){ //If run not in filter list, events pass 
      new_filter=true; 
      pass = new_filter && pass_goodv && pass_eebadsc && pass_hbhe && pass_hbheiso&& pass_jets;
      newtree->Fill();
      continue;      
    }
    else if(imap->second.find(event)==imap->second.end()){ //If event not in filter list, event passes
      new_filter=true; 
      pass = new_filter && pass_goodv && pass_eebadsc && pass_hbhe && pass_hbheiso&& pass_jets;
      newtree->Fill();
      continue;
    }
    else{ //If run and event are in filter list, event fails
      new_filter=false;
      pass=false;
      newtree->Fill();
    }
  }
  newtree->AutoSave();
  newtreeglobal->AutoSave();
  delete oldfile;
  delete newfile;
}
