      SUBROUTINE  DRGLG(D, DR, IV, LIV, LV, N, ND, NN, P, PS, R,
     1                  RD, RHO, RHOI, RHOR, V, X)
C
C *** ITERATION DRIVER FOR GENERALIZED (NON)LINEAR MODELS (ETC.)
C
      INTEGER LIV, LV, N, ND, NN, P, PS
      INTEGER IV(LIV), RHOI(*)
      DOUBLE PRECISION D(P), DR(ND,N), R(*), RD(*), RHOR(*),
     1                 V(LV), X(*)
C     DIMENSION RD(N, (P-PS)*(P-PS+1)/2 + 1)
      EXTERNAL RHO
C
C--------------------------  PARAMETER USAGE  --------------------------
C
C D....... SCALE VECTOR.
C DR...... DERIVATIVES OF R AT X.
C IV...... INTEGER VALUES ARRAY.
C LIV..... LENGTH OF IV... LIV MUST BE AT LEAST P + 90.
C LV...... LENGTH OF V...  LV  MUST BE AT LEAST
C              105 + P*(2*P+16) + 2*N + 4*PS.
C N....... TOTAL NUMBER OF RESIDUALS.
C ND...... LEADING DIMENSION OF DR -- MUST BE AT LEAST PS.
C NN...... LEAD DIMENSION OF R, RD.
C P....... NUMBER OF PARAMETERS (COMPONENTS OF X) BEING ESTIMATED.
C PS...... NUMBER OF NON-NUISANCE PARAMETERS.
C R....... RESIDUALS (OR MEANS -- FUNCTIONS OF X) WHEN  DRGLG IS CALLED
C          WITH IV(1) = 1.
C RD...... RD(I) = HALF * (G(I)**T * H(I)**-1 * G(I)) ON OUTPUT WHEN
C          IV(RDREQ) IS 2, 3, 5, OR 6.   DRGLG SETS IV(REGD) = 1 IF RD
C          IS SUCCESSFULLY COMPUTED, TO 0 IF NO ATTEMPT WAS MADE
C          TO COMPUTE IT, AND TO -1 IF H (THE FINITE-DIFFERENCE HESSIAN)
C          WAS INDEFINITE.  BEFORE CONVERGENCE, RD IS ALSO USED AS
C          TEMPORARY STORAGE.
C RHO..... COMPUTES INFO ABOUT OBJECTIVE FUNCTION.
C RHOI.... PASSED WITHOUT CHANGE TO RHO.
C RHOR.... PASSED WITHOUT CHANGE TO RHO.
C V....... FLOATING-POINT VALUES ARRAY.
C X....... PARAMETER VECTOR BEING ESTIMATED (INPUT = INITIAL GUESS,
C              OUTPUT = BEST VALUE FOUND).
C
C *** CALLING SEQUENCE FOR RHO...
C
C  CALL RHO(NEED, F, N, NF, XN, R, RD, RHOI, RHOR, W)
C
C  PARAMETER DECLARATIONS FOR RHO...
C
C INTEGER NEED(2), N, NF, RHOI(*)
C FLOATING-POINT F, XN(*), R(*), RD(N,*), RHOR(*), W(N)
C
C    RHOI AND RHOR ARE FOR RHO TO USE AS IT SEES FIT.  THEY ARE PASSED
C TO RHO WITHOUT CHANGE.  IF IV(RDREQ) IS AT LEAST 4, I.E., IF MORE
C THAN THE SIMPLEST REGRESSION DIAGNOSTIC INFORMATION IS TO BE COMPUTED,
C THEN SOME COMPONENTS OF RHOI AND RHOR MUST CONVEY SOME EXTRA
C DETAILS, AS DESCRIBED BELOW.
C    F, R, RD, AND W ARE EXPLAINED BELOW WITH NEED.
C    XN IS THE VECTOR OF NUISANCE PARAMETERS (OF LENGTH P - PS).  IF
C RHO NEEDS TO KNOW THE LENGTH OF XN, THEN THIS LENGTH SHOULD BE
C COMMUNICATED THROUGH RHOI (OR THROUGH COMMON).  RHO SHOULD NOT CHANGE
C XN.
C    NEED(1) = 1 MEANS RHO SHOULD SET F TO THE SUM OF THE LOSS FUNCTION
C VALUES AT THE RESIDUALS R(I).  NF IS THE CURRENT FUNCTION INVOCATION
C COUNT (A VALUE THAT IS INCREMENTED EACH TIME A NEW PARAMETER EXTIMATE
C X IS CONSIDERED).  NEED(2) IS THE VALUE NF HAD AT THE LAST R WHERE
C RHO MIGHT BE CALLED WITH NEED(1) = 2.  IF RHO SAVES INTERMEDIATE
C RESULTS FOR USE IN CALLS WITH NEED(1) = 2, THEN IT CAN USE NF TO TELL
C WHICH INTERMEDIATE RESULTS ARE APPROPRIATE, AND IT CAN SAVE SOME OF
C THESE RESULTS IN R.
C    NEED(1) = 2 MEANS RHO SHOULD SET R(I) TO THE LOSS FUNCTION
C DERIVATIVE WITH RESPECT TO THE RESIDUALS THAT WERE PASSED TO RHO WHEN
C NF HAD THE SAME VALUE IT DOES NOW (AND NEED(1) WAS 1).  RHO SHOULD
C ALSO SET W(I) TO THE APPROXIMATION OF THE SECOND DERIVATIVE OF THE
C LOSS FUNCTION (WITH RESPECT TO THE I-TH RESIDUAL) THAT SHOULD BE USED
C IN THE GAUSS-NEWTON MODEL.  WHEN THERE ARE NUISANCE PARAMETERS (I.E.,
C WHEN PS .LT. P) RHO SHOULD ALSO SET R(I+K*N) TO THE DERIVATIVE OF THE
C LOSS FUNCTION WITH RESPECT TO THE I-TH RESIDUAL AND XN(K), AND IT
C SHOULD SET RD(I,J + K*(K+1)/2 + 1) TO THE SECOND PARTIAL DERIVATIVE
C OF THE I-TH RESIDUAL WITH RESPECT TO XN(J) AND XN(K), 0 .LE. J .LE. K
C AND 1 .LE. K .LE. P - PS, WHERE XN(0) MEANS THE I-TH RESIDUAL ITSELF.
C IN ANY EVENT, RHO SHOULD ALSO SET RD(I,1) TO THE (TRUE) SECOND
C DERIVATIVE OF THE LOSS FUNCTION WITH RESPECT TO THE I-TH RESIDUAL.
C    NF (THE FUNCTION INVOCATION COUNT WHOSE NORMAL USE IS EXPLAINED
C ABOVE) SHOULD NOT BE CHANGED UNLESS RHO CANNOT CARRY OUT THE REQUESTED
C TASK, IN WHICH CASE RHO SHOULD SET NF TO 0.
C
C
C  ***  REGRESSION DIAGNOSTICS  ***
C
C IV(RDREQ) INDICATES WHETHER A COVARIANCE MATRIX AND REGRESSION
C DIAGNOSTIC VECTOR ARE TO BE COMPUTED.  IV(RDREQ) HAS THE FORM
C IV(RDREQ) = CVR +2*RDR, WHERE CVR = 0 OR 1 AND RDR = 0, 1, OR 2,
C SO THAT
C
C      CVR = MOD(IV(RDREQ), 2)
C      RDR = MOD(IV(RDREQ)/2, 3).
C
C    CVR = 0 FOR NO COVARIANCE MATRIX
C        = 1 IF A COVARIANCE MATRIX ESTIMATE IS DESIRED
C
C    RDR = 0 FOR NO LEAVE-ONE-OUT DIAGNOSTIC INFORMATION.
C        = 1 TO HAVE ONE-STEP ESTIMATES OF F(X(I)) - F(X*) STORED IN RD,
C            WHERE X(I) MINIMIZES F (THE OBJECTIVE FUNCTION) WITH
C            COMPONENT I OF R REMOVED AND X* MINIMIZES THE FULL F.
C        = 2 FOR MORE DETAILED ONE-STEP LEAVE-ONE-OUT INFORMATION, AS
C            DICTATED BY THE IV COMPONENTS DESCRIBED BELOW.
C
C FOR RDR = 2, THE FOLLOWING COMPONENTS OF IV ARE RELEVANT...
C
C  NFIX = IV(83) = NUMBER OF TRAILING NUISANCE PARAMETERS TO TREAT
C          AS FIXED WHEN COMPUTING DIAGNOSTIC VECTORS (0 .LE. NFIX .LE.
C          P - PS, SO X(I) IS KEPT FIXED FOR P - NFIX .LT. I .LE. P).
C
C   LOO = IV(84) TELLS WHAT TO LEAVE OUT...
C       = 1 MEANS LEAVE OUT EACH COMPONENT OF R SEPARATELY, AND
C       = 2 MEANS LEAVE OUT CONTIGUOUS BLOCKS OF R COMPONENTS.
C           FOR LOO = 2, IV(85) IS THE STARTING SUBSCRIPT IN RHOI
C           OF AN ARRAY BS OF BLOCK SIZES, IV(86) IS THE STRIDE FOR BS,
C           AND IV(87) = NB IS THE NUMBER OF BLOCKS, SO THAT
C           BS(I) = RHOI(IV(85) + (I-1)*IV(86)), 1 .LE. I .LE. NB.
C           NOTE THAT IF ALL BLOCKS ARE THE SAME SIZE, THEN IT SUFFICES
C           TO SET RHOI(IV(85)) = BLOCKSIZE AND IV(86) = 0.
C           NOTE THAT LOO = 1 IS EQUIVALENT TO LOO = 2 WITH
C           RHOI(IV(85)) = 1, IV(86) = 0, IV(87) = N.
C       = 3,4 ARE SIMILAR TO LOO = 1,2, RESPECTIVELY, BUT LEAVING A
C           FRACTION OUT.  IN THIS CASE, IV(88) IS THE STARTING
C           SUBSCRIPT IN RHOR OF AN ARRAY FLO OF FRACTIONS TO LEAVE OUT,
C           AND IV(89) IS THE STRIDE FOR FLO...
C           FLO(I) = RHOR(IV(88) + (I-1)*IV(89)), 1 .LE. I .LE. NB.
C
C XNOTI = IV(90) TELLS WHAT DIAGNOSTIC INFORMATION TO STORE...
C       = 0  MEANS JUST STORE ONE-STEP ESTIMATES OF F(X(I)) - F(X*) IN
C            RD(I), 1 .LE. I .LE. NB.
C       .GT. 0 MEANS ALSO STORE ONE-STEP ESTIMATES OF X(I) ESTIMATES
C            IN RHOR, STARTING AT RHOR(XNOTI)...
C              X(I)(J) = RHOR((I-1)*(P-NFIX) + J + XNOTI-1),
C              1 .LE. I .LE. NB, 1 .LE. J .LE. P - NFIX.
C
C    SOMETIMES ONE-STEP ESTIMATES OF X(I) DO NOT EXIST, BECAUSE THE
C APPROXIMATE UPDATED HESSIAN MATRIX IS INDEFINITE.  IN SUCH CASES,
C THE CORRESPONDING RD COMPONENT IS SET TO -1, AND, IF XNOTI IS
C POSITIVE, THE SOLUTION X IS RETURNED AS X(I).  WHEN ONE-STEP ESTIMATES
C OF X(I) DO EXIST, THE CORRESPONDING COMPONENT OF RD IS POSITIVE.
C
C SUMMARY OF RHOI COMPONENTS (FOR RDR = MOD(IV(RDREQ)/2, 3) = 2)...
C
C IV(83) = NFIX
C IV(84) = LOO
C IV(85) = START IN RHOI OF BS
C IV(86) = STRIDE FOR BS
C IV(87) = NB
C IV(88) = START IN RHOR OF FLO
C IV(89) = STRIDE FOR FLO
C IV(90) = XNOTI (START IN RHOR OF X(I)).
C
C
C  ***  COVARIANCE MATRIX ESTIMATE  ***
C
C IF IV(RDREQ) INDICATES THAT A COVARIANCE MATRIX IS TO BE COMPUTED,
C THEN IV(COVREQ) = IV(15) DETERMINES THE FORM OF THE COMPUTED
C COVARIANCE MATRIX ESTIMATE AND, SIMULTANEOUSLY, THE FORM OF
C APPROXIMATE HESSIAN MATRIX USED IN COMPUTING REGRESSION DIAGNOSTIC
C INFORMATION.  IN ALL CASES, SOME APPROXIMATE FINAL HESSIAN MATRIX
C IS OBTAINED, AND ITS INVERSE IS THE COVARIANCE MATRIX ESTIMATE
C (WHICH MAY HAVE TO BE SCALED APPROPRIATELY -- THAT IS UP TO YOU).
C IF IV(COVREQ) IS AT MOST 2 IN ABSOLUTE VALUE, THEN THE FINAL
C HESSIAN APPROXIMATION IS COMPUTED BY FINITE DIFFERENCES -- GRADIENT
C DIFFERENCES IF IV(COVREQ) IS NONNEGATIVE, FUNCTION DIFFERENCES
C OTHERWISE.  IF (IV(COVREQ)) IS AT LEAST 3 IN ABSOLUTE VALUE, THEN THE
C CURRENT GAUSS-NEWTON HESSIAN APPROXIMATION IS TAKEN AS THE FINAL
C HESSIAN APPROXIMATION.  FOR SOME PROBLEMS THIS SAVES TIME AND YIELDS
C THE SAME OR NEARLY THE SAME HESSIAN APPROXIMATION AS DO FINITE
C DIFFERENCES.  FOR OTHER PROBLEMS, THE TWO KINDS OF HESSIAN
C APPROXIMATIONS MAY GIVE DECIDEDLY DIFFERENT REGRESSION DIAGNOSTICS AND
C COVARIANCE MATRIX ESTIMATES.
C
C
C  ***  GENERAL  ***
C
C     CODED BY DAVID M. GAY.
C
C+++++++++++++++++++++++++++++  DECLARATIONS  ++++++++++++++++++++++++++
C
C  ***  EXTERNAL FUNCTIONS AND SUBROUTINES  ***
C
      EXTERNAL DD7UP5,DIVSET, DG2LRD, DN3RDP, DD7TPR, DQ7ADR, DVSUM,
     1        DG7LIT,DITSUM, DL7NVR, DL7ITV, DL7IVM,DL7SRT, DL7SQR,
     2         DL7SVX, DL7SVN, DL7TSQ,DL7VML,DO7PRD,DV2AXY,DV7CPY,
     3         DV7SCL, DV7SCP
      DOUBLE PRECISION DD7TPR, DL7SVX, DL7SVN,DVSUM
C
C DD7UP5...  UPDATES SCALE VECTOR D.
C DIVSET.... PROVIDES DEFAULT IV AND V INPUT COMPONENTS.
C DG2LRD.... COMPUTES REGRESSION DIAGNOSTIC.
C DN3RDP... PRINTS REGRESSION DIAGNOSTIC.
C DD7TPR... COMPUTES INNER PRODUCT OF TWO VECTORS.
C DQ7ADR.... ADDS ROWS TO QR FACTORIZATION.
C DVSUM..... RETURNS SUM OF ELEMENTS OF A VECTOR.
C DG7LIT.... PERFORMS BASIC MINIMIZATION ALGORITHM.
C DITSUM.... PRINTS ITERATION SUMMARY, INFO ABOUT INITIAL AND FINAL X.
C DL7NVR... INVERTS COMPACTLY STORED TRIANGULAR MATRIX.
C DL7ITV... MULTIPLIES INVERSE TRANSPOSE OF LOWER TRIANGLE TIMES VECTOR.
C DL7IVM... APPLY INVERSE OF COMPACT LOWER TRIANG. MATRIX.
C DL7SRT.... COMPUTES CHOLESKY FACTOR OF (LOWER TRIANG. OF) SYM. MATRIX.
C DL7SQR... COMPUTES L*(L**T) FOR LOWER TRIANG. MATRIX L.
C DL7SVX... UNDERESTIMATES LARGEST SINGULAR VALUE OF TRIANG. MATRIX.
C DL7SVN... OVERESTIMATES SMALLEST SINGULAR VALUE OF TRIANG. MATRIX.
C DL7TSQ... COMPUTES (L**T)*L FOR LOWER TRIANG. MATRIX L.
C DL7VML.... COMPUTES L * V, V = VECTOR, L = LOWER TRIANGULAR MATRIX.
C DO7PRD.... ADDS OUTER PRODUCT OF VECTORS TO A MATRIX.
C DV2AXY.... ADDS A MULTIPLE OF ONE VECTOR TO ANOTHER.
C DV7CPY.... COPIES ONE VECTOR TO ANOTHER.
C DV7SCL... MULTIPLIES A VECTOR BY A SCALAR.
C DV7SCP... SETS ALL ELEMENTS OF A VECTOR TO A SCALAR.
C
C  ***  LOCAL VARIABLES  ***
C
      LOGICAL JUSTG, UPDATD, ZEROG
      INTEGER G1, HN1, I, II, IV1, J, J1, JTOL1, K, L, LH,
     1        NEED1(2), NEED2(2),  PMPS, PS1, PSLEN, QTR1,
     2        RMAT1, STEP1, TEMP1, TEMP2, TEMP3, TEMP4, W, WI, Y1
      DOUBLE PRECISION RHMAX, RHTOL, RHO1, RHO2, T
      double precision tmp
C
      DOUBLE PRECISION ONE, ZERO
C
C  ***  SUBSCRIPTS FOR IV AND V  ***
C
      INTEGER CNVCOD, COVMAT, DINIT, DTYPE, DTINIT, D0INIT, F,
     1        F0, FDH, G, H, HC, IPIVOT, IVNEED, JCN, JTOL, LMAT,
     2        MODE, NEXTIV, NEXTV, NF0, NF1, NFCALL, NFCOV, NFGCAL,
     3        NGCALL, NGCOV, PERM, QTR, RDREQ, REGD, RESTOR,
     4        RMAT, RSPTOL, STEP, TOOBIG, VNEED, XNOTI, Y
C
C  ***  IV SUBSCRIPT VALUES  ***
C
      PARAMETER (CNVCOD=55, COVMAT=26, DTYPE=16, F0=13, FDH=74, G=28,
     1           H=56, HC=71, IPIVOT=76, IVNEED=3, JCN=66, JTOL=59,
     2           LMAT=42, MODE=35, NEXTIV=46, NEXTV=47, NFCALL=6,
     3           NFCOV=52, NF0=68, NF1=69, NFGCAL=7, NGCALL=30,
     4           NGCOV=53, PERM=58, QTR=77, RESTOR=9, RMAT=78, RDREQ=57,
     5           REGD=67, STEP=40, TOOBIG=2, VNEED=4, XNOTI=90, Y=48)
C
C  ***  V SUBSCRIPT VALUES  ***
C
      PARAMETER (DINIT=38, DTINIT=39, D0INIT=40, F=10, RSPTOL=49)
      PARAMETER (ONE=1.D+0, ZERO=0.D+0)
      SAVE NEED1, NEED2
      DATA NEED1(1)/1/, NEED1(2)/0/, NEED2(1)/2/, NEED2(2)/0/
C
C+++++++++++++++++++++++++++++++  BODY  ++++++++++++++++++++++++++++++++
C
      LH = P * (P+1) / 2
      IF (IV(1) .EQ. 0) CALL DIVSET(1, IV, LIV, LV, V)
      PS1 = PS + 1
      IV1 = IV(1)
      IF (IV1 .GT. 2) GO TO 10
         W = IV(Y) + P
         IV(RESTOR) = 0
         I = IV1 + 2
         IF (IV(TOOBIG) .EQ. 0) GO TO (120, 110, 110, 130), I
         V(F) = V(F0)
         IF (I .NE. 3) IV(1) = 2
         GO TO 40
C
C  ***  FRESH START OR RESTART -- CHECK INPUT INTEGERS  ***
C
 10   IF (ND .LT. PS) GO TO 360
      IF (PS .GT. P) GO TO 360
      IF (PS .LE. 0) GO TO 360
      IF (N .LE. 0) GO TO 360
      IF (IV1 .EQ. 14) GO TO 30
      IF (IV1 .GT. 16) GO TO 420
      IF (IV1 .LT. 12) GO TO 40
      IF (IV1 .EQ. 12) IV(1) = 13
      IF (IV(1) .NE. 13) GO TO 20
      IV(IVNEED) = IV(IVNEED) + P
      IV(VNEED) = IV(VNEED) + P*(P+13)/2 + 2*N + 4*PS
C     *** ADJUST IV(PERM) TO MAKE ROOM FOR IV INPUT COMPONENTS
C     *** NEEDED WHEN IV(RDREQ) IS 4 OR 5...
      I = XNOTI + 1
      IF (IV(PERM) .LT. I) IV(PERM) = I
C
 20   CALL DG7LIT(D, X, IV, LIV, LV, P, PS, V, X, X)
      IF (IV(1) .NE. 14) GO TO 999
C
C  ***  STORAGE ALLOCATION  ***
C
      IV(IPIVOT) = IV(NEXTIV)
      IV(NEXTIV) = IV(IPIVOT) + P
      IV(Y) = IV(NEXTV)
      IV(G) = IV(Y) + P + N
      IV(RMAT) = IV(G) + P + 4*PS
      IV(QTR) = IV(RMAT) + LH
      IV(JTOL) = IV(QTR) + P + N
      IV(JCN) = IV(JTOL) + 2*P
      IV(NEXTV) = IV(JCN) + P
      IF (IV1 .EQ. 13) GO TO 999
C
 30   JTOL1 = IV(JTOL)
      IF (V(DINIT) .GE. ZERO) CALL DV7SCP(P, D, V(DINIT))
      IF (V(DTINIT) .GT. ZERO) CALL DV7SCP(P, V(JTOL1), V(DTINIT))
      I = JTOL1 + P
      IF (V(D0INIT) .GT. ZERO) CALL DV7SCP(P, V(I), V(D0INIT))
      IV(NF0) = 0
      IV(NF1) = 0
C
 40   G1 = IV(G)
      Y1 = IV(Y)
      CALL DG7LIT(D, V(G1), IV, LIV, LV, P, PS, V, X, V(Y1))
      IF (IV(1) - 2) 50, 60, 380
C
 50   V(F) = ZERO
      IF (IV(NF1) .EQ. 0) GO TO 999
      IF (IV(RESTOR) .NE. 2) GO TO 999
      IV(NF0) = IV(NF1)
      CALL DV7CPY(N, RD, R)
      IV(REGD) = 0
      GO TO 999
C
 60   IF (IV(MODE) .GT. 0) GO TO 370
      CALL DV7SCP(P, V(G1), ZERO)
      RMAT1 = IABS(IV(RMAT))
      QTR1 = IABS(IV(QTR))
      CALL DV7SCP(PS, V(QTR1), ZERO)
      IV(REGD) = 0
      CALL DV7SCP(PS, V(Y1), ZERO)
      CALL DV7SCP(LH, V(RMAT1), ZERO)
      IF (IV(RESTOR) .NE. 3) GO TO 70
         CALL DV7CPY(N, R, RD)
         IV(NF1) = IV(NF0)
   70 itmp = iv(nfgcal)
      CALL RHO(NEED2, T, N, itmp, X(PS1), R, RD, RHOI, RHOR,
     $     V(W))
      iv(nfgcal) = itmp
      IF (IV(NFGCAL) .GT. 0) GO TO 90
 80      IV(TOOBIG) = 1
         GO TO 40
 90   IF (IV(MODE) .LT. 0) GO TO 999
      DO 100 I = 1, N
 100     CALL DV2AXY(PS, V(Y1), R(I), DR(1,I), V(Y1))
      GO TO 999
C
C  ***  COMPUTE F(X)  ***
C
 110  I = IV(NFCALL)
      NEED1(2) = IV(NFGCAL)
      tmp = v(f)
      CALL RHO(NEED1, tmp, N, I, X(PS1), R, RD, RHOI, RHOR, V(W))
      v(f) = tmp
      IV(NF1) = I
      IF (I .LE. 0) GO TO 80
      GO TO 40
C
C  ***  COMPUTE GRADIENT INFORMATION FOR FINITE-DIFFERENCE HESSIAN  ***
C
 120  IV(1) = 2
      JUSTG = .TRUE.
      I = IV(NFCALL)
      CALL RHO(NEED1, T, N, I, X(PS1), R, RD, RHOI, RHOR, V(W))
      IF (I .LE. 0) GO TO 80
      CALL RHO(NEED2, T, N, I, X(PS1), R, RD, RHOI, RHOR, V(W))
      IF (I .LE. 0) GO TO 80
      GO TO 250
C
C  ***  PREPARE TO COMPUTE GRADIENT INFORMATION WHILE ITERATING  ***
C
 130  JUSTG = .FALSE.
      G1 = IV(G)
C
C  ***  DECIDE WHETHER TO UPDATE D BELOW  ***
C
      I = IV(DTYPE)
      UPDATD = .FALSE.
      IF (I .LE. 0) GO TO 140
         IF (I .EQ. 1 .OR. IV(MODE) .LT. 0) UPDATD = .TRUE.
C
C  ***  COMPUTE RMAT AND QTR  ***
C
 140  QTR1 = IABS(IV(QTR))
      RMAT1 = IABS(IV(RMAT))
      IV(RMAT) = RMAT1
      IV(HC) = 0
      IV(NF0) = 0
      IV(NF1) = 0
      IF (IV(MODE) .LT. 0) GO TO 160
C
C  ***  ADJUST Y  ***
C
      Y1 = IV(Y)
      WI = W
      STEP1 = IV(STEP)
      DO 150 I = 1, N
         T = V(WI) - RD(I)
         WI = WI + 1
         IF (T .NE. ZERO) CALL DV2AXY(PS, V(Y1),
     1                    T*DD7TPR(PS,V(STEP1),DR(1,I)), DR(1,I), V(Y1))
 150     CONTINUE
C
C  ***  CHECK FOR NEGATIVE W COMPONENTS  ***
C
 160  J1 = W + N - 1
      DO 170 WI = W, J1
         IF (V(WI) .LT. ZERO) GO TO 240
 170     CONTINUE
C
C  ***  W IS NONNEGATIVE.  COMPUTE QR FACTORIZATION  ***
C  ***  AND, IF NECESSARY, USE SEMINORMAL EQUATIONS  ***
C
      RHMAX = ZERO
      RHTOL = V(RSPTOL)
      TEMP1 = G1 + P
      ZEROG = .TRUE.
      WI = W
      DO 200 I = 1, N
         RHO1 = R(I)
         RHO2 = V(WI)
         WI = WI + 1
         T =  SQRT(RHO2)
         IF (RHMAX .LT. RHO2) RHMAX = RHO2
         IF (RHO2 .GT. RHTOL*RHMAX) GO TO 180
C           *** SEMINORMAL EQUATIONS ***
            CALL DV2AXY(PS, V(G1), RHO1, DR(1,I), V(G1))
            RHO1 = ZERO
            ZEROG = .FALSE.
            GO TO 190
 180     RHO1 =  RHO1 / T
C        *** QR ACCUMULATION ***
 190     CALL DV7SCL(PS, V(TEMP1), T, DR(1,I))
         CALL DQ7ADR(PS, V(QTR1), V(RMAT1), V(TEMP1), RHO1)
 200     CONTINUE
C
C  ***  COMPUTE G FROM RMAT AND QTR  ***
C
      TEMP2 = TEMP1 + PS
      CALL DL7VML(PS, V(TEMP1), V(RMAT1), V(QTR1))
      IF (ZEROG) GO TO 220
      IV(QTR) = -QTR1
      IF (DL7SVX(PS, V(RMAT1), V(TEMP2), V(TEMP2)) * RHTOL .GE.
     1    DL7SVN(PS, V(RMAT1), V(TEMP2), V(TEMP2))) GO TO 230
         CALL DL7IVM(PS, V(TEMP2), V(RMAT1), V(G1))
C
C        *** SEMINORMAL EQUATIONS CORRECTION OF BJOERCK --
C        *** ONE CYCLE OF ITERATIVE REFINEMENT...
C
         TEMP3 = TEMP2 + PS
         TEMP4 = TEMP3 + PS
         CALL DL7ITV(PS, V(TEMP3), V(RMAT1), V(TEMP2))
         CALL DV7SCP(PS, V(TEMP4), ZERO)
         RHMAX = ZERO
         WI = W
         DO 210 I = 1, N
            RHO2 = V(WI)
            WI = WI + 1
            IF (RHMAX .LT. RHO2) RHMAX = RHO2
            RHO1 = ZERO
            IF (RHO2 .LE. RHTOL*RHMAX) RHO1 = R(I)
            T = RHO1 - RHO2*DD7TPR(PS, V(TEMP3), DR(1,I))
            CALL DV2AXY(PS, V(TEMP4), T, DR(1,I), V(TEMP4))
 210        CONTINUE
         CALL DL7IVM(PS, V(TEMP3), V(RMAT1), V(TEMP4))
         CALL DV2AXY(PS, V(TEMP2), ONE, V(TEMP3), V(TEMP2))
         CALL DV2AXY(PS, V(QTR1), ONE, V(TEMP2), V(QTR1))
 220     IV(QTR) = QTR1
 230  CALL DV2AXY(PS, V(G1), ONE, V(TEMP1), V(G1))
      IF (PS .GE. P) GO TO 350
      GO TO 270
C
C  ***  INDEFINITE GN HESSIAN...  ***
C
 240  IV(RMAT) = -RMAT1
      IV(HC) = RMAT1
      CALL DO7PRD(N, LH, PS, V(RMAT1), V(W), DR, DR)
C
C  ***  COMPUTE GRADIENT  ***
C
 250  G1 = IV(G)
      CALL DV7SCP(P, V(G1), ZERO)
      DO 260 I = 1, N
 260     CALL DV2AXY(PS, V(G1), R(I), DR(1,I), V(G1))
      IF (PS .GE. P) GO TO 350
C
C  ***  COMPUTE GRADIENT COMPONENTS OF NUISANCE PARAMETERS ***
C
 270  K = P - PS
      J1 = 1
      G1 = G1 + PS
      DO 280 J = 1, K
         J1 = J1 + NN
         V(G1) =DVSUM(N, R(J1))
         G1 = G1 + 1
 280     CONTINUE
      IF (JUSTG) GO TO 390
C
C  ***  COMPUTE HESSIAN COMPONENTS OF NUISANCE PARAMETERS  ***
C
      I = PS*PS1/2
      PSLEN = P*(P+1)/2 - I
      HN1 = RMAT1 + I
      CALL DV7SCP(PSLEN, V(HN1), ZERO)
      PMPS = P - PS
      K = HN1
      J1 = 1
      DO 310 II = 1, PMPS
         J1 = J1 + NN
         J = J1
         DO 290 I = 1, N
            CALL DV2AXY(PS, V(K), RD(J), DR(1,I), V(K))
            J = J + 1
 290        CONTINUE
         K = K + PS
         DO 300 I = 1, II
            J1 = J1 + NN
            V(K) =DVSUM(N, RD(J1))
            K = K + 1
 300        CONTINUE
 310     CONTINUE
      IF (IV(RMAT) .LE. 0) GO TO 350
      J = IV(LMAT)
      CALL DV7CPY(PSLEN, V(J), V(HN1))
      IF (DL7SVN(PS, V(RMAT1), V(TEMP2), V(TEMP2)) .LE. ZERO) GO TO 320
      CALL DL7SRT(PS1, P, V(RMAT1), V(RMAT1), I)
      IF (I .LE. 0) GO TO 330
C
C  *** HESSIAN IS NOT POSITIVE DEFINITE ***
C
 320  CALL DL7SQR(PS, V(RMAT1), V(RMAT1))
      CALL DV7CPY(PSLEN, V(HN1), V(J))
      IV(HC) = RMAT1
      IV(RMAT) = -RMAT1
      GO TO 350
C
C  *** NUISANCE PARS LEAVE HESSIAN POS. DEF.  GET REST OF QTR ***
C
 330  J = QTR1 + PS
      G1 = IV(G) + PS
      DO 340 I = PS1, P
         T = DD7TPR(I-1, V(HN1), V(QTR1))
         HN1 = HN1 + I
         V(J) = (V(G1) - T) / V(HN1-1)
         J = J + 1
         G1 = G1 + 1
 340     CONTINUE
 350  IF (JUSTG) GO TO 390
      IF (UPDATD) CALL DD7UP5(D, IV, LIV, LV, P, PS, V)
      GO TO 40
C
C  ***  MISC. DETAILS  ***
C
C     ***  BAD N, ND, OR P  ***
C
 360  IV(1) = 66
      GO TO 420
C
C  ***  COVARIANCE OR INITIAL S COMPUTATION  ***
C
 370  IV(NFCOV) = IV(NFCOV) + 1
      IV(NFCALL) = IV(NFCALL) + 1
      IV(NFGCAL) = IV(NFCALL)
      IV(1) = -1
      GO TO 999
C
C  ***  CONVERGENCE OBTAINED -- SEE WHETHER TO COMPUTE COVARIANCE  ***
C
 380  IF (IV(COVMAT) .NE. 0) GO TO 410
      IF (IV(REGD) .NE. 0) GO TO 410
C
C     ***  SEE IF CHOLESKY FACTOR OF HESSIAN IS AVAILABLE  ***
C
      K = IV(FDH)
      IF (K .LE. 0) GO TO 400
      IF (IV(RDREQ) .LE. 0) GO TO 410
C
C     ***  COMPUTE REGRESSION DIAGNOSTICS AND DEFAULT COVARIANCE IF
C          DESIRED  ***
C
      IV(MODE) = P + 1
      IV(NGCALL) = IV(NGCALL) + 1
      IV(NGCOV) = IV(NGCOV) + 1
      IV(CNVCOD) = IV(1)
      IV(NFCOV) = IV(NFCOV) + 1
      IV(NFCALL) = IV(NFCALL) + 1
      IV(NFGCAL) = IV(NFCALL)
      IV(1) = -1
      GO TO 999
C
 390  IF (IV(MODE) .LE. P) GO TO 40
C     *** SAVE RD IN W FOR POSSIBLE USE IN OTHER DIAGNOSTICS ***
      CALL DV7CPY(N, V(W), RD)
C     *** OVERWRITE RD WITH REGRESSION DIAGNOSTICS ***
      L = IV(LMAT)
      I = IV(JCN)
      STEP1 = IV(STEP)
      CALL DG2LRD(DR, IV, V(L), LH, LIV, LV, ND, N, P, PS, R, RD,
     1            RHOI, RHOR, V, V(STEP1), X, V(I))
      IV(1) = IV(CNVCOD)
      IV(CNVCOD) = 0
      IF (MOD(IV(RDREQ),2) .EQ. 0) GO TO 410
C
C        *** FINISH COVARIANCE COMPUTATION ***
C
         I = IABS(IV(H))
         IV(FDH) = 0
         CALL DL7NVR(P, V(I), V(L))
         CALL DL7TSQ(P, V(I), V(I))
         IV(COVMAT) = I
         GO TO 410
C
C  ***  COME HERE FOR INDEFINITE FINITE-DIFFERENCE HESSIAN  ***
C
 400  IV(COVMAT) = K
      IV(REGD) = K
C
C  ***  PRINT SUMMARY OF FINAL ITERATION AND OTHER REQUESTED ITEMS  ***
C
 410  G1 = IV(G)
 420  CALL DITSUM(D, V(G1), IV, LIV, LV, P, V, X)
      IF (IV(1) .LE. 6 .AND. IV(RDREQ) .GT. 0)
     1     CALL DN3RDP(IV, LIV, LV, N, P, RD, RHOI, RHOR, V)
C
 999  RETURN
C  ***  LAST LINE OF  DRGLG FOLLOWS  ***
      END
