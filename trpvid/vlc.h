
/*!
 ************************************************************************
 * \file vlc.h
 *
 * \brief
 *    header for (CA)VLC coding functions
 *
 * \author
 *    Karsten Suehring
 *
 ************************************************************************
 */

#ifndef _VLC_H_
#define _VLC_H_

int se_v ( Bitstream *bitstream );
int ue_v ( Bitstream *bitstream );
int u_v ( int LenInBits, Bitstream *bitstream );
int u_1 ( Bitstream *bitstream );

// UVLC mapping
void linfo_ue(int len, int info, int *value1, int *dummy);
void linfo_se(int len, int info, int *value1, int *dummy);

int  readSyntaxElement_VLC (SyntaxElement *sym, Bitstream *currStream);
int  readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);
int  readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);

int  GetVLCSymbol (uns8b buffer[],int totbitoffset,int *info, int bytecount);
int  GetVLCSymbol_IntraMode (uns8b buffer[],int totbitoffset,int *info, int bytecount);

int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream);
int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym,  DataPartition *dP,
                                           char *type);
int readSyntaxElement_NumCoeffTrailingOnesChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Level_VLC0(SyntaxElement *sym, struct datapartition *dP);
int readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, struct datapartition *dP);
int readSyntaxElement_TotalZeros(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_TotalZerosChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Run(SyntaxElement *sym,  DataPartition *dP);
int GetBits (uns8b buffer[],int totbitoffset,int *info, int bytecount, 
             int numbits);
int ShowBits (uns8b buffer[],int totbitoffset,int bytecount, int numbits);

int more_rbsp_data (uns8b buffer[],int totbitoffset,int bytecount);


#endif

