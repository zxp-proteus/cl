      IMPLICIT NONE
      INCLUDE 'O8PARA.INC'
      REAL  RUNTIM
      REAL  OPTITE
      DOUBLE PRECISION ACCINF(0:MAXIT,32)
      INTEGER ITSTEP,PHASE
      COMMON/O8ITIN/OPTITE,ITSTEP,PHASE,RUNTIM,
     F      ACCINF
      INCLUDE 'O8XDAT.INC'
      DOUBLE PRECISION GRADF,GFN,QGF,GPHI0,GPHI1,GRES,GRESN
      COMMON/O8GRD/GRADF(NX),GFN,QGF(NX),GRES(NX,NRESM),
     F        GRESN(NRESM),GPHI0(NX),GPHI1(NX)
C**
      INCLUDE 'O8RDAT.INC'
      LOGICAL VAL,GCONST,LLOW,LUP
      INTEGER GUNIT
      COMMON/O8GRI/VAL(0:NRESM),GCONST(0:NRESM),GUNIT(3,0:NRESM),
     F             LLOW(NX),LUP(NX)
      LOGICAL INTAKT,INX,STD,TE0,TE1,TE2,TE3,SINGUL
      LOGICAL IDENT,EQRES,SILENT,ANALYT,COLD
      COMMON/O8STPA/INTAKT,INX,STD,TE0,TE1,TE2,TE3,SINGUL,
     F        IDENT,EQRES,SILENT,ANALYT,COLD
      DOUBLE PRECISION A,SCALM,SCALM2,DIAG0,MATSC
      COMMON/O8QN/A(NX,NX),DIAG0(NX),
     F           SCALM,SCALM2,MATSC
      INTEGER VIOLIS,ALIST,BIND,BIND0,SORT(NRESM)
      COMMON/O8RESI/BIND(NRESM),BIND0(NRESM),VIOLIS(0:NSTEP*NRESM),
     F               ALIST(0:NRESM),SORT
      DOUBLE PRECISION U,U0,W,W1,RES,RES0,RES1,RESST,SCF,SCF0,
     F                YU,SLACK,INFEAS,WORK
      COMMON/O8RESD/RES(NRESM),RES0(NRESM),RES1(NRESM),RESST(NRESM),
     F            U(NRESM),U0(NRESM),W(NRESM),W1(NRESM),WORK(0:NRESM),
     F            YU(NRESM),SLACK(NRESM),SCF,SCF0,INFEAS
      INTEGER N,NR,NRES,NH,NG
      COMMON/O8DIM/N,NH,NG,NR,NRES
      DOUBLE PRECISION EPSMAC,TOLMAC,DELDIF
      COMMON/O8MPAR/EPSMAC,TOLMAC,DELDIF
      INTEGER ITERMA,IFILL1
      DOUBLE PRECISION DEL,DEL0,DEL01,DELMIN,TAU0,TAU,NY
      DOUBLE PRECISION SMALLD,SMALLW,RHO,RHO1,ETA,EPSX,EPSPHI,C1D,
     F                 SCFMAX,UPDMY0,TAUQP,TAUFAC,TAUMAX
      COMMON/O8PAR/DEL0,DEL01,DEL,DELMIN,TAU0,TAU,
     F             SMALLD,SMALLW,RHO,RHO1,ETA,NY,EPSX,EPSPHI,
     F             C1D,SCFMAX,TAUQP,TAUFAC,TAUMAX,UPDMY0,
     F             ITERMA,IFILL1
      DOUBLE PRECISION ALPHA,BETA,THETA,SIGSM,SIGLA,DELTA,STPTRM
      DOUBLE PRECISION DELTA1,STMAXL
      COMMON/O8STEP/ALPHA,BETA,THETA,SIGSM,SIGLA,DELTA,STPTRM,
     F              DELTA1,STMAXL
      INTEGER ICF,ICGF,CFINCR,CRES,CGRES
      COMMON/O8CNT/ICF,ICGF,CFINCR,CRES(NRESM),CGRES(NRESM)
      LOGICAL FFUERR,CFUERR(NRESM)
      COMMON/O8ERR/FFUERR,CFUERR
      DOUBLE PRECISION LEVEL
      INTEGER CLOW,LASTDW,LASTUP,LASTCH
      COMMON/O8WEI/LEVEL,CLOW,LASTDW,LASTUP,LASTCH
      CHARACTER*40 NAME
      COMMON/O8ID/NAME
      DOUBLE PRECISION EPSDIF
      COMMON/O8DER/EPSDIF
      INTEGER PROU,MEU
      COMMON/O8IO/PROU,MEU
      DOUBLE PRECISION UG(NX),OG(NX),DELFAC(NRESM)
      COMMON/O8BD/UG,OG,DELFAC
      INTEGER NRESET,NUMSM
      COMMON/O8RST/NRESET,NUMSM
      DOUBLE PRECISION XST(NX)
      COMMON/O8STV/XST
      INTRINSIC SQRT,EXP,LOG,MAX,MIN,ABS

