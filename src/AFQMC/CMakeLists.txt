
SET(AFQMC_DEBUG 3)

#-------------------------------------------------------------------
# Sources
#-------------------------------------------------------------------
SET (AFQMC_SRCS 
  AFQMCFactory.cpp
  Walkers/LocalWalkerHandler.cpp
  Walkers/DistWalkerHandler.cpp
  Drivers/AFQMCDriver.cpp 
  Drivers/selectedCI.cpp 
  Drivers/VMCDriver.cpp 
  Hamiltonians/SparseGeneralHamiltonian.cpp
  Wavefunctions/WavefunctionHandler.cpp
  Wavefunctions/PureSingleDeterminant.cpp
  Wavefunctions/MultiPureSingleDeterminant.cpp
  Wavefunctions/GeneralSingleDeterminant.cpp
  #Wavefunctions/MultiGeneralSingleDeterminant.cpp
  Propagators/phaseless_ImpSamp_ForceBias.cpp
  Propagators/VMCPropagator.cpp
  Numerics/SparseMatrixOperations.cpp
  Numerics/DenseMatrixOperations.cpp
  Sandbox/compare_libraries.cpp
  Wavefunctions/WavefunctionHelper.cpp
  Hamiltonians/DDProjector.cpp
  Hamiltonians/CCProjector.cpp
  Utilities/Utils.cpp
  ) 

INCLUDE_DIRECTORIES(/usr/gapps/qmc/libs/INTEL/eigen-eigen-10219c95fe65/)
ADD_LIBRARY(afqmc ${AFQMC_SRCS})

