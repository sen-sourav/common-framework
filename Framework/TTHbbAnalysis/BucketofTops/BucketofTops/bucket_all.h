#include <stdio.h>
#include <math.h>
#include <string>
#include <vector>
#include "TLorentzVector.h"
#include "PathResolver/PathResolver.h"
using namespace std;

//put top bucket algo stuff inside bucketAlgo namespace
namespace bucketAlgo
{

  double mTop = 173.5; //GeV
  double mW = 80.4; //GeV

  std::vector <TLorentzVector> compvec(std::vector <TLorentzVector> EVT, std::vector <TLorentzVector> set) //another bug
  {
    std::vector <TLorentzVector> compset;
    for (int i = 0; i < EVT.size(); ++i)
    {
      bool findflag = false;
      for (int j = 0; j < set.size(); ++j)
      {
        findflag = findflag || (EVT[i]==set[j]); 
      }
      if (!findflag) {compset.push_back(EVT[i]);}
    }
    return compset;
  };

  //power set generator
  std::vector <std::vector <int> > pSet(std::vector <int> set, int offset=0)
  {
      //power set size : 2^n
      unsigned int pow_set_size = pow(2, set.size());
      int counter;
      std::vector <std::vector <int> > pVec;
      //Run counter from 000..1 and stop before 111..1 (111..1 excluded)
      for(counter = 1; counter < pow_set_size-offset; counter++)
      {
        std::vector <int> v;
        for(int j = 0; j < set.size(); j++)
        {
            //if jth bit in the counter is 1 print the jth element
            if(counter & (1<<j))
              v.push_back(set[j]);
        }
        pVec.push_back(v); 
        v.clear();
      }
      return pVec;
  }
 
  
  //complementary set generator
  std::vector <int> cSet(std::vector <int> set, std::vector <int> subset)
  {
    std::vector <int> cVec;
    for (int i = 0; i < set.size(); ++i)
    {
       if (find(subset.begin(), subset.end(), set[i]) != subset.end())
       {
         continue;
       }
       cVec.push_back(set[i]);
    }
    return cVec;
  };
  


  //class for bucket object
  class bucket
  {
  private:
    string bucket_label;
    double Mbucket, pTbucket, etabucket;
    double mpairnum, ratio_min;
  public:
    std::vector <TLorentzVector> members;
    std::vector <TLorentzVector> nonBJETS;
    TLorentzVector BJET;

  ~bucket() {} //destructor

  bucket()
  {
    bucket_label = "tx"; //tx means label unassigned
    Mbucket = -9999; //GeV
    pTbucket = -9999; //GeV
    etabucket = -9999;
    mpairnum = -9999; //GeV
  }

  bucket(std::vector <TLorentzVector> nonbjets, TLorentzVector bjet)
  {
    nonBJETS = nonbjets;
    BJET = bjet;
    members.push_back(bjet);
    TLorentzVector b;
    b = b + bjet;
    for(int i =0; i < nonbjets.size(); ++i)
    {
      b = b + nonbjets[i];
      members.push_back(nonbjets[i]);
    }
    Mbucket = b.M();
    pTbucket = b.Pt();
    etabucket = b.Eta();
  } 

  void setBucketLabel(string label)  {bucket_label=label;}

  string getBucketLabel() {return bucket_label;}

  double getBucketMass() {return Mbucket;}

  double getBucketPt() {return pTbucket;}

  double getBucketEta() {return etabucket;}

  std::vector<int> getOrderlist()
  {
    std::vector<int> orderlist;
    for (int i=0; i < members.size(); ++i)
    {
      orderlist.push_back(i+1);
    }
    return orderlist;
  }

  bool twflag() //true : tw ; false : not tw
  {
    ratio_min = pow(10, 10); 
    bool flag = false; //defalt bucket not tw
    int sizeB = members.size();


    for (int i = 0; i < sizeB; ++i)
    {
      for (int j = 0; j < sizeB; ++j)
      {
        if (j < i) {continue;}
	//cout << "(i,j): " << i << "\t" << j << endl;
	TLorentzVector temp = members[i]+members[j];
        double dd = abs((temp.M()/Mbucket) - (mW/mTop));
	//cout << "dd: " << dd << "\tratmin: " << ratio_min << endl;
        if (ratio_min > dd) //
        {
          ratio_min = dd;
          mpairnum = temp.M();
	  //^//cout << "temp_mass_W: " << mpairnum << "\tupdating_mass_ratio: " << ratio_min << endl;
        }
      }
    }
    //^//cout << "final_mass_ratio: " << ratio_min << endl;
    if ( (ratio_min) < 0.15 ) {flag = true;}
    return flag;
  }

  double WcandMnum() {return mpairnum;}
  
  double WcandRatio() {return ratio_min;}

  double twOptMetric() {return (Mbucket - mTop)*(Mbucket - mTop);} 

  double tminusOptMetric()
  {
    if (Mbucket > 155.0) {return pow(10,10);}
    else {return abs(Mbucket - 145.0);}
  }

  };


  //function for the extra jets
  std::vector <TLorentzVector> extra(std::vector<TLorentzVector> specbjets, std::vector<TLorentzVector> specnonbjets, std::vector <bucketAlgo::bucket> Blist)
  {
    std::vector <TLorentzVector> Xmembers;
    Xmembers.reserve( specbjets.size() + specnonbjets.size() );
    Xmembers.insert( Xmembers.end(), specbjets.begin(), specbjets.end() );
    Xmembers.insert( Xmembers.end(), specnonbjets.begin(), specnonbjets.end() );
    for (int i = 0; i < Blist.size(); ++i)
    {
      Xmembers = bucketAlgo::compvec(Xmembers, Blist[i].members);
    }
    return Xmembers;
  };
  





  //function to get two top buckets
  std::vector <bucketAlgo::bucket> doublebucket(std::vector<TLorentzVector> specbjets, std::vector<TLorentzVector> specnonbjets, double MbucketMax, double MbucketMin, string target_label, double B1weight)
  {
    std::vector <bucketAlgo::bucket> Blist; //B1 and B2
    bucketAlgo::bucket B1, B2;
    int nonbjetsize = specnonbjets.size();
    std::vector <std::vector <int> > nonbindexset1;
    std::vector <int> nonbset;
    for (int j = 0; j < nonbjetsize; ++j) {nonbset.push_back(j);}
    if (target_label == "tw")
    {
      nonbindexset1 = pSet(nonbset); //no offset for tw
    }
    else //for t- pair search 
    {
      for (int l1 = 0; l1 < nonbjetsize; ++l1)
      {
        std::vector <int> tempPar1;
        tempPar1.push_back(nonbset[l1]);
        nonbindexset1.push_back(tempPar1);
        tempPar1.clear();
      }
    }
    double Deltatw = pow(10,10); //arbit large number
    for (int i = 0; i < nonbindexset1.size(); ++i) //looping over all possible buckets
    {
      std::vector <std::vector <int> > nonbindexset2;
      std::vector <int> restjetset = cSet(nonbset, nonbindexset1[i]);
      /*switching of second bucket's freedom for now//nonbindexset2 = pSet(restjetset); // no offset as all the jets can now be used*/
      if (target_label == "tw") 
      {
        nonbindexset2.push_back(restjetset);
      }
      else //for t- pair search
      {
        for (int l2 = 0; l2 < restjetset.size(); ++l2)
        {
          std::vector <int> tempPar2;
          tempPar2.push_back(restjetset[l2]);
          nonbindexset2.push_back(tempPar2);
          tempPar2.clear();
        }
      }
      std::vector <TLorentzVector> nonbA;
      for (int k = 0; k < nonbindexset1[i].size(); ++k)
      {
        nonbA.push_back(specnonbjets[nonbindexset1[i][k]]);
      }
      for (int i1 = 0; i1 < nonbindexset2.size(); ++i1)
      {
        std::vector <TLorentzVector> nonbB;
        for (int k1 = 0; k1 < nonbindexset2[i1].size(); ++k1)
        {
          nonbB.push_back(specnonbjets[nonbindexset2[i1][k1]]);
        }
        bucketAlgo::bucket Afirst(nonbA, specbjets[0]);
        double AfirstDistance = (target_label == "tw") ? Afirst.twOptMetric() : Afirst.tminusOptMetric();
        std::vector <int> pidlAfirst = Afirst.getOrderlist(); //
        bucketAlgo::bucket Asecond(nonbA, specbjets[1]);
	double AsecondDistance = (target_label == "tw") ? Asecond.twOptMetric() : Asecond.tminusOptMetric();
        std::vector <int> pidlAsecond = Asecond.getOrderlist(); //
        bucketAlgo::bucket Bfirst(nonbB, specbjets[0]);
	double BfirstDistance = (target_label == "tw") ? Bfirst.twOptMetric() : Bfirst.tminusOptMetric();
        std::vector <int> pidlBfirst = Bfirst.getOrderlist(); //
        bucketAlgo::bucket Bsecond(nonbB, specbjets[1]);
	double BsecondDistance = (target_label == "tw") ? Bsecond.twOptMetric() : Bsecond.tminusOptMetric();
        std::vector <int> pidlBsecond = Bsecond.getOrderlist(); //
        
        double del1 = (B1weight*AfirstDistance) + BsecondDistance;
        double del2 = (B1weight*BfirstDistance) + AsecondDistance;
        double del3 = AfirstDistance + (B1weight*BsecondDistance);
        double del4 = BfirstDistance + (B1weight*AsecondDistance);
        if ((del1 < del2) && (del1 < del3) && (del1 < del4))
        {
          if (del1 < Deltatw)
          {
            Deltatw = del1;
            B1 = Afirst;
            B2 = Bsecond;
          }
        }
        else if ((del2 < del1) && (del2 < del3) && (del2 < del4))
        {
          if (del2 < Deltatw)
          {
            Deltatw = del2;
            B1 = Bfirst;
            B2 = Asecond;
          }
        }
        else if ((del3 < del1) && (del3 < del2) && (del3 < del4))
        {
          if (del3 < Deltatw)
          {
            Deltatw = del3;
            B1 = Bsecond;
            B2 = Afirst;
          }
        }
        else 
        {
          if (del4 < Deltatw)
          {
            Deltatw = del4;
            B1 = Asecond;
            B2 = Bfirst;
          }
        }



      }
    } // loop over all possible buckets ends
    Blist.push_back(B1);
    Blist.push_back(B2);
    for (int i = 0; i < Blist.size(); ++i)
    {
      string label; //label assignement , 
      double Bm = Blist[i].getBucketMass();
      if (target_label == "tw")
      {
        label = (Blist[i].twflag())?"tw":"t-";
      }
      else {label = "t-";}
      if ((Bm > MbucketMax) || (Bm < MbucketMin))
      {
        label = "t0";
      }
      Blist[i].setBucketLabel(label);
    }
    return Blist;
  };


  //function to get one top bucket
  bucketAlgo::bucket singlebucket(std::vector<TLorentzVector> specbjets, std::vector<TLorentzVector> specnonbjets, bucketAlgo::bucket twbucket, double MbucketMax, double MbucketMin)
  {

    bucketAlgo::bucket newbucket; 
    double Deltamin = pow(10,10); //arbit large number

    TLorentzVector bucketBjet = (specbjets[0] == twbucket.BJET) ? specbjets[1] : specbjets[0]; //bjet to be used in the bucket

    int Evnonbjetsize = specnonbjets.size();
    int twnonbjetsize = twbucket.nonBJETS.size();
    for (int i = 0; i < Evnonbjetsize; ++i)
    {
      bool findtw = false;
      for (int j = 0; j < twnonbjetsize; ++j)
      {
        bool findtemp = (specnonbjets[i]==twbucket.nonBJETS[j]); //particle part of tw bucket; can be added to extra
        findtw = findtw || findtemp;
      }
      if (!findtw)  //no match
      {
        std::vector <TLorentzVector> tempnb;
        tempnb.push_back(specnonbjets[i]);
        bucketAlgo::bucket tempbucket(tempnb, bucketBjet);
        if (Deltamin > tempbucket.tminusOptMetric())
        {
          Deltamin = tempbucket.tminusOptMetric();
          newbucket = tempbucket;
        }
      }
    }
    
    //label assignement
    double massbucket = newbucket.getBucketMass();
    string label = ((massbucket < MbucketMax) && (massbucket  > MbucketMin)) ? "t-" : "t0";
    newbucket.setBucketLabel(label);
    return newbucket;
  };



//namespace ends
}