//////////////////////////////////////////////////////////////////
// (c) Copyright 2008-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef QMCPLUSPLUS_DENSITY_HAMILTONIAN_H
#define QMCPLUSPLUS_DENSITY_HAMILTONIAN_H
#include <QMCHamiltonians/QMCHamiltonianBase.h>
#include <OhmmsPETE/OhmmsArray.h>

namespace qmcplusplus 
{

  class DensityEstimator: public QMCHamiltonianBase
  {
    public:

    DensityEstimator(ParticleSet& elns);

    void resetTargetParticleSet(ParticleSet& P);

    Return_t evaluate(ParticleSet& P);

    inline Return_t evaluate(ParticleSet& P, vector<NonLocalData>& Txy) 
    {
      return evaluate(P);
    }

    void addObservables(PropertySetType& plist,BufferType& olist);
    void registerCollectables(vector<observable_helper*>& h5desc, hid_t gid) const ;
    void setObservables(PropertySetType& plist);
    void setParticlePropertyList(PropertySetType& plist, int offset);
    bool put(xmlNodePtr cur);
    bool get(std::ostream& os) const;
    QMCHamiltonianBase* makeClone(ParticleSet& qp, TrialWaveFunction& psi);

    inline int getGridIndex(int i, int j, int k) const
    {
      return myIndex+k+NumGrids[2]*(j+NumGrids[1]*i);
    }

    private:
    ///number of grids
    TinyVector<int,OHMMS_DIM+1> NumGrids;
    ///bounds
    TinyVector<RealType,OHMMS_DIM> Bounds;
    ///bin size
    TinyVector<RealType,OHMMS_DIM> Delta;
    ///inverse
    TinyVector<RealType,OHMMS_DIM> DeltaInv;
    ///name of the density data
    string prefix;
    ///density
    Array<RealType,OHMMS_DIM> density;
    /** resize the internal data
     *
     * The argument list is not completed
     */
    void resize();
  };

}
#endif

/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 2945 $   $Date: 2008-08-05 10:21:33 -0500 (Tue, 05 Aug 2008) $
 * $Id: ForceBase.h 2945 2008-08-05 15:21:33Z jnkim $ 
 ***************************************************************************/