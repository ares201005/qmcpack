//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
#include "OhmmsData/AttributeSet.h"
#include "Numerics/HDFSTLAttrib.h"
#include "QMCWaveFunctions/TricubicBsplineSetBuilder.h"
#include "QMCWaveFunctions/ElectronGasOrbitalBuilder.h"
#include "QMCWaveFunctions/HEGGrid.h"
#include "Message/Communicate.h"
//#define DEBUG_BSPLINE_EG
namespace qmcplusplus {

  /** Specialization for Array<double,3> */
  template<>
    struct HDFAttribIO<Array<double,3> >: public HDFAttribIOBase 
    {
      typedef Array<double,3> ArrayType_t;
      ArrayType_t&  ref;
      HDFAttribIO<ArrayType_t>(ArrayType_t& a):ref(a) { }
      inline void write(hid_t grp, const char* name) 
      {
      }

      inline void read(hid_t grp, const char* name) 
      {
        std::vector<int> npts(3);
        npts[0]=ref.size(0);
        npts[1]=ref.size(1);
        npts[2]=ref.size(2);
        HDFAttribIO<std::vector<double> > dummy(ref.storage(),npts);
        dummy.read(grp,name);
      }
    };


  TricubicBsplineSetBuilder::TricubicBsplineSetBuilder(ParticleSet& p, PtclPoolType& psets):
    targetPtcl(p),ptclPool(psets),LowerBox(0.0),UpperBox(1.0),BoxGrid(2){
      for(int idim=0; idim<DIM; idim++)
        UpperBox[idim]=targetPtcl.Lattice.R(idim,idim);
  }   

  /** Initialize the cubic grid
   */
  bool TricubicBsplineSetBuilder::put(xmlNodePtr cur){

    cur=cur->children;
    while(cur != NULL)
    {
      std::string cname((const char*)(cur->name));
      if(cname == "grid") {
        int idir=0,npts=2; 
        RealType ri=0,rf=-1.0;
        OhmmsAttributeSet aAttrib;
        aAttrib.add(idir,"dir");
        aAttrib.add(ri,"ri");
        aAttrib.add(rf,"rf");
        aAttrib.add(npts,"npts");
        aAttrib.put(cur);
        //bound check
        RealType del=rf-ri;
        if(del>0 && del<UpperBox[idir]) UpperBox[idir]=rf;
        LowerBox[idir]=ri;
        BoxGrid[idir]=npts;
      }
      cur=cur->next;
    }
    app_log() << "  TricubicBsplineSetBuilder set a global box " << endl;
    app_log() << "    Lower box " << LowerBox << endl;
    app_log() << "    Upper box " << UpperBox << endl;
    app_log() << "    Box grid "  << BoxGrid << endl;
    return true;
  }

  /** create a SlaterDeterminant
   * @param cur xmlnode containing \<slaterdeterminant\>
   * @return a SlaterDeterminant
   *
   * @warning MultiSlaterDeterminant is not working yet.
   */
  SPOSetBase*
  TricubicBsplineSetBuilder::createSPOSet(xmlNodePtr cur){

    string hrefname("NONE");
    int norb(0);
    int degeneracy(1);
    OhmmsAttributeSet aAttrib;
    aAttrib.add(norb,"orbitals");
    aAttrib.add(degeneracy,"degeneracy");
    aAttrib.add(hrefname,"href");
    aAttrib.put(cur);

    if(norb ==0) {
      app_error() << "TricubicBsplineSetBuilder::createSPOSet failed. Check the attribte orbitals." << endl;
      return 0;
    }

    app_log() << "    Only valid for spin-unpolarized cases " << endl;
    app_log() << "    Degeneracy = " << degeneracy << endl;
    StorageType inData(BoxGrid[0],BoxGrid[1],BoxGrid[2]);

    SPOSetType* psi= new SPOSetType(norb);
    vector<int> occSet(norb);
    for(int i=0; i<norb; i++) occSet[i]=i;

    cur=cur->children;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == "occupation") {
        string occ_mode("ground");
        const xmlChar* o=xmlGetProp(cur,(const xmlChar*)"mode");  
        if(o!= NULL) occ_mode = (const char*)o;
        //Do nothing if mode == ground
        if(occ_mode == "excited") {
          vector<int> occ_in, occRemoved;
          putContent(occ_in,cur);
          for(int k=0; k<occ_in.size(); k++) {
            if(occ_in[k]<0) 
              occRemoved.push_back(-occ_in[k]-1);
          }
          int kpopd=0;
          for(int k=0; k<occ_in.size(); k++) {
            if(occ_in[k]>0) 
              occSet[occRemoved[kpopd++]]=occ_in[k]-1;
          }
        }


        const xmlChar* h5path = xmlGetProp(cur,(const xmlChar*)"h5path");
        string hroot("/eigenstates_3/twist_0");
        if(h5path != NULL) hroot=(const char*)h5path;
        char wfname[128],wfshortname[16];
        
        map<string,OrbitalGroupType*>::iterator git(myBasis.find("0"));
        OrbitalGroupType *abasis=0;
        if(git == myBasis.end())
        {
          abasis = new OrbitalGroupType;
          abasis->setGrid(LowerBox[0],UpperBox[0],LowerBox[1],UpperBox[1],LowerBox[2],UpperBox[2],
              BoxGrid[0]-1,BoxGrid[1]-1,BoxGrid[2]-1);
          myBasis["0"]=abasis;
        } 
        else
        {
          abasis=(*git).second;
        }

        hid_t h_file = H5Fopen(hrefname.c_str(),H5F_ACC_RDWR,H5P_DEFAULT);
        for(int iorb=0; iorb<norb; iorb++) {
          sprintf(wfname,"%s/band_%d/eigenvector",hroot.c_str(),occSet[iorb]/degeneracy);
          sprintf(wfshortname,"b%d",occSet[iorb]/degeneracy);
          StorageType* newP=0;
          map<string,StorageType*>::iterator it(BigDataSet.find(wfshortname));
          if(it == BigDataSet.end()) {
            newP=new StorageType;
            HDFAttribIO<StorageType> dummy(inData);
            dummy.read(h_file,wfname);
            BigDataSet[wfshortname]=newP;
            abasis->add(iorb,inData,newP);
            app_log() << "   Reading spline function " << wfname << endl;
          } else {
            app_log() << "   Reusing spline function " << wfname << endl;
          }
        }
        psi->add(abasis);
        H5Fclose(h_file);
      }
      cur=cur->next;
    }
    return psi;
  }

  /** Create B-spline for the electron-gas wavefunctions
   *
   * This function is to test bspline against analytic electron-gas
   * single-particle orbitals.
   */
  SPOSetBase* TricubicBsplineSetBuilder::createSPOSetWithEG()
  {
    int norb=7;
    std::vector<int> npts(3);
    npts[0]=BoxGrid[0]-1; 
    npts[1]=BoxGrid[1]-1;
    npts[2]=BoxGrid[2]-1;
    StorageType inData(npts[0],npts[1],npts[2]);

    RealType dx=(UpperBox[0]-LowerBox[0])/static_cast<RealType>(npts[0]);
    RealType dy=(UpperBox[1]-LowerBox[1])/static_cast<RealType>(npts[1]);
    RealType dz=(UpperBox[2]-LowerBox[2])/static_cast<RealType>(npts[2]);

    SPOSetType* psi= new SPOSetType(norb);
    map<string,OrbitalGroupType*>::iterator git(myBasis.find("0"));
    OrbitalGroupType *abasis=0;
    if(git == myBasis.end())
    {
      abasis = new OrbitalGroupType;
      abasis->setGrid(LowerBox[0],UpperBox[0],LowerBox[1],UpperBox[1],LowerBox[2],UpperBox[2],
          npts[0],npts[1],npts[2]);
      myBasis["0"]=abasis;

      int nc=1;
      int nat=targetPtcl.getTotalNum();
      int nup=nat/2;
      HEGGrid<RealType,OHMMS_DIM> egGrid(targetPtcl.Lattice);
      int nkpts=(nup-1)/2;

      cout << "Number of kpoints " << nkpts << endl;
      //create a E(lectron)G(as)O(rbital)Set
      egGrid.createGrid(nc,nkpts);
      RealEGOSet* eg=new RealEGOSet(egGrid.kpt,egGrid.mk2); 
      char wfshortname[16];
      for(int iorb=0; iorb<norb; iorb++) {
        sprintf(wfshortname,"b%d",iorb);
        for(int ix=0; ix<npts[0]; ix++) {
          double x(dx*ix+LowerBox[0]);
          for(int iy=0; iy<npts[1]; iy++) {
            double y(dy*iy+LowerBox[1]);
            double z(LowerBox[2]);
            for(int iz=0; iz<npts[2]; iz++) {
              inData(ix,iy,iz)=eg->f(PosType(x,y,z),iorb); z+=dz;
            }
          }
        }
        StorageType* newP=0;
        map<string,StorageType*>::iterator it(BigDataSet.find(wfshortname));
        if(it == BigDataSet.end()) {
          newP=new StorageType;
          BigDataSet[wfshortname]=newP;
          abasis->add(iorb,inData,newP);
          app_log() << "   Using spline function for EG " << wfshortname << endl;
        } 
      }
      delete eg;
    } 
    else
    {
      abasis=(*git).second;
    }

    psi->add(abasis);
    return psi;
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
