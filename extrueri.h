
/*****************************************************************************
                       �W���b�������W�J���C�u����
                                                      �ŏI�X�V 2000/09/14
 ----------------------------------------------------------------------------
             Copyright (C) 2000 L.Entis. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *****************************************************************************/


#if	!defined(EXTRUERI_H_INCLUDED)
#define	EXTRUERI_H_INCLUDED


/*****************************************************************************
                         �������A���P�[�V����
 *****************************************************************************/

extern	PVOID eriAllocateMemory( DWORD dwBytes ) ;
extern	void eriFreeMemory( PVOID pMemory ) ;


/*****************************************************************************
                             ���Z�֐�
 *****************************************************************************/

extern	DWORD ChopMulDiv( DWORD x, DWORD y, DWORD z ) ;
extern	DWORD RaiseMulDiv( DWORD x, DWORD y, DWORD z ) ;


/*****************************************************************************
                                �摜���
 *****************************************************************************/

struct	ERI_FILE_HEADER
{
	DWORD	dwVersion ;
	DWORD	dwContainedFlag ;
	DWORD	dwKeyFrameCount ;
	DWORD	dwFrameCount ;
	DWORD	dwAllFrameTime ;
} ;

struct	ERI_INFO_HEADER
{
	DWORD	dwVersion ;
	DWORD	fdwTransformation ;
	DWORD	dwArchitecture ;
	DWORD	fdwFormatType ;
	SDWORD	nImageWidth ;
	SDWORD	nImageHeight ;
	DWORD	dwBitsPerPixel ;
	DWORD	dwClippedPixel ;
	DWORD	dwSamplingFlags ;
	SDWORD	dwQuantumizedBits[2] ;
	DWORD	dwAllottedBits[2] ;
	DWORD	dwBlockingDegree ;
	DWORD	dwLappedBlock ;
	DWORD	dwFrameTransform ;
	DWORD	dwFrameDegree ;
} ;

struct	RASTER_IMAGE_INFO
{
	DWORD	fdwFormatType ;
	PBYTE	ptrImageArray ;
	SDWORD	nImageWidth ;
	SDWORD	nImageHeight ;
	DWORD	dwBitsPerPixel ;
	SDWORD	BytesPerLine ;
} ;

#define	ERI_RGB_IMAGE		0x00000001
#define	ERI_RGBA_IMAGE		0x04000001
#define	ERI_GRAY_IMAGE		0x00000002
#define	ERI_TYPE_MASK		0x00FFFFFF
#define	ERI_WITH_PALETTE	0x01000000
#define	ERI_USE_CLIPPING	0x02000000
#define	ERI_WITH_ALPHA		0x04000000


/*****************************************************************************
                   ���������O�X�E�K���}�E�R���e�L�X�g
 *****************************************************************************/

class	RLHDecodeContext
{
protected:
	int		m_flgZero ;			// �[���t���O
	ULONG	m_nLength ;			// ���������O�X
	BYTE	m_bytIntBufCount ;	// ���ԓ��̓o�b�t�@�ɒ~�ς���Ă���r�b�g��
	BYTE	m_bytIntBuffer ;	// ���ԓ��̓o�b�t�@
	ULONG	m_nBufferingSize ;	// �o�b�t�@�����O����o�C�g��
	ULONG	m_nBufCount ;		// �o�b�t�@�̎c��o�C�g��
	PBYTE	m_ptrBuffer ;		// ���̓o�b�t�@�̐擪�ւ̃|�C���^
	PBYTE	m_ptrNextBuf ;		// ���ɓǂݍ��ނׂ����̓o�b�t�@�ւ̃|�C���^

	struct	ARITHCODE_SYMBOL
	{
		WORD	wOccured ;					// �V���{���̏o����
		SWORD	wSymbol ;					// �V���{��
	} ;
	struct	STATISTICAL_MODEL
	{
		DWORD				dwTotalSymbolCount ;	// �S�V���{���� < 10000H
		ARITHCODE_SYMBOL	SymbolTable[1] ;		// ���v���f��
	} ;
	DWORD	m_dwCodeRegister ;		// �R�[�h���W�X�^
	DWORD	m_dwAugendRegister ;	// �I�[�W�F���h���W�X�^
	DWORD	m_dwCodeBuffer ;		// �r�b�g�o�b�t�@
	int		m_nVirtualPostBuf ;		// ���z�o�b�t�@�J�E���^
	int		m_nSymbolBitCount ;		// �����̃r�b�g��
	int		m_nSymbolSortCount ;	// �����̑���
	int		m_maskSymbolBit ;		// �����̃r�b�g�}�X�N
	STATISTICAL_MODEL *	m_pLastStatisticalModel ;		// �Ō�̓��v���f��
	STATISTICAL_MODEL *	m_pStatisticalTable[0x100] ;	// ���v���f��

	struct	MTF_TABLE
	{
		BYTE	iplt[0x100] ;

		inline void Initialize( void ) ;
		inline BYTE MoveToFront( int index ) ;
	} ;
	MTF_TABLE *	m_pMTFTable[0x101] ;	// MTF�e�[�u��

	typedef	ULONG (RLHDecodeContext::*PFUNC_DECODE_SYMBOLS)
									( PINT ptrDst, ULONG nCount ) ;
	PFUNC_DECODE_SYMBOLS	m_pfnDecodeSymbols ;

public:
	// �\�z�֐�
	RLHDecodeContext( ULONG nBufferingSize ) ;
	// ���Ŋ֐�
	virtual ~RLHDecodeContext( void ) ;

	// �[���t���O��ǂݍ���ŁA�R���e�L�X�g������������
	inline void Initialize( void ) ;
	// �o�b�t�@����̎��A���̃f�[�^��ǂݍ���
	inline int PrefetchBuffer( void ) ;
	// 1�r�b�g�擾����i 0 ���́|1��Ԃ� �j
	inline INT GetABit( void ) ;
	// n�r�b�g�擾����
	inline UINT GetNBits( int n ) ;
	// �K���}�R�[�h���擾����
	inline INT GetACode( void ) ;
	// ���̃f�[�^��ǂݍ���
	virtual ULONG ReadNextData( PBYTE ptrBuffer, ULONG nBytes ) = 0 ;

	// �Z�p�����̕����̏�������
	void PrepareToDecodeArithmeticCode( int nBitCount ) ;
	// RL-MTF-G �����̕����̏���������
	void PrepareToDecodeRLMTFGCode( void ) ;

	// �W�J�����f�[�^���擾����
	inline ULONG DecodeSymbols( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeGammaCodes( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeArithmeticCodes( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeRLMTFGCodes( PBYTE ptrDst, ULONG nCount ) ;

} ;

//
// MTF_TABLE ������������
//////////////////////////////////////////////////////////////////////////////
inline void RLHDecodeContext::MTF_TABLE::Initialize( void )
{
	for ( int i = 0; i < 0x100; i ++ )
		iplt[i] = (BYTE) i ;
}

//
// Move To Front �����s
//////////////////////////////////////////////////////////////////////////////
inline BYTE RLHDecodeContext::MTF_TABLE::MoveToFront( int index )
{
	BYTE	pplt = iplt[index] ;
	while ( index > 0 )
	{
		iplt[index] = iplt[index - 1] ;
		index -- ;
	}
	return	(iplt[0] = pplt) ;
}

//
// �[���t���O��ǂݍ���ŁA�R���e�L�X�g������������
//////////////////////////////////////////////////////////////////////////////
inline void RLHDecodeContext::Initialize( void )
{
	m_flgZero = (int) GetABit( ) ;
	m_nLength = 0 ;
}

//
// �o�b�t�@����̎��A���̃f�[�^��ǂݍ���
//////////////////////////////////////////////////////////////////////////////
// �Ԃ�l�G
//		0�̎��A����I��
//		1�̎��A�ُ�I��
//////////////////////////////////////////////////////////////////////////////
inline int RLHDecodeContext::PrefetchBuffer( void )
{
	if ( m_bytIntBufCount == 0 )
	{
		if ( m_nBufCount == 0 )
		{
			m_ptrNextBuf = m_ptrBuffer;
			m_nBufCount = ReadNextData( m_ptrBuffer, m_nBufferingSize ) ;
			if ( m_nBufCount == 0 )
				return	1 ;
		}
		m_bytIntBufCount = 8;
		m_bytIntBuffer = *(m_ptrNextBuf ++) ;
		m_nBufCount -- ;
	}
	return	0 ;
}

//
// 1�r�b�g���擾����
//////////////////////////////////////////////////////////////////////////////
// �Ԃ�l�G
//		0 ���́|1��Ԃ��B
//		�G���[�����������ꍇ��1��Ԃ��B
//////////////////////////////////////////////////////////////////////////////
inline INT RLHDecodeContext::GetABit( void )
{
	if ( PrefetchBuffer( ) )
		return	1 ;
	//
	INT	nValue = - (INT)(m_bytIntBuffer >> 7) ;
	m_bytIntBufCount -- ;
	m_bytIntBuffer <<= 1 ;
	return	nValue ;
}

//
// n�r�b�g�擾����
//////////////////////////////////////////////////////////////////////////////
inline UINT RLHDecodeContext::GetNBits( int n )
{
	UINT	nCode = 0 ;
	while ( n != 0 )
	{
		if ( PrefetchBuffer( ) )
			break ;
		//
		int	nCopyBits = n;
		if ( nCopyBits > m_bytIntBufCount )
			nCopyBits = m_bytIntBufCount;
		//
		nCode = (nCode << nCopyBits) | (m_bytIntBuffer >> (8 - nCopyBits)) ;
		n -= nCopyBits;
		m_bytIntBufCount -= nCopyBits;
		m_bytIntBuffer <<= nCopyBits;
	}
	return	nCode ;
}

//
// �K���}�R�[�h���擾����
//////////////////////////////////////////////////////////////////////////////
// �Ԃ�l�G
//		�擾���ꂽ�K���}�R�[�h��Ԃ��B
//		�G���[�����������ꍇ�́|1��Ԃ��B
//////////////////////////////////////////////////////////////////////////////
inline INT RLHDecodeContext::GetACode( void )
{
	INT	nCode = 0, nBase = 1 ;
	//
	for ( ; ; )
	{
		//
		// �o�b�t�@�ւ̃f�[�^�̓ǂݍ���
		if ( PrefetchBuffer( ) )
			return	(LONG) -1 ;
		//
		// 1�r�b�g���o��
		m_bytIntBufCount -- ;
		if ( !(m_bytIntBuffer & 0x80) )
		{
			//
			// �R�[�h�I��
			nCode += nBase ;
			m_bytIntBuffer <<= 1 ;
			return	nCode ;
		}
		else
		{
			//
			// �R�[�h�p��
			nBase <<= 1 ;
			m_bytIntBuffer <<= 1 ;
			//
			// �o�b�t�@�ւ̃f�[�^�̓ǂݍ���
			if ( PrefetchBuffer( ) )
				return	(LONG) -1 ;
			//
			// 1�r�b�g���o��
			nCode = (nCode << 1) | (m_bytIntBuffer >> 7) ;
			m_bytIntBufCount -- ;
			m_bytIntBuffer <<= 1 ;
		}
	}
}

//
// �W�J�����f�[�^���擾����
//////////////////////////////////////////////////////////////////////////////
inline ULONG RLHDecodeContext::DecodeSymbols( PINT ptrDst, ULONG nCount )
{
	return	(this->*m_pfnDecodeSymbols)( ptrDst, nCount ) ;
}


/*****************************************************************************
                               �W�J�I�u�W�F�N�g
 *****************************************************************************/

class	ERIDecoder
{
protected:
	ERI_INFO_HEADER		m_EriInfHdr ;			// �摜���w�b�_

	UINT				m_nBlockSize ;			// �u���b�L���O�T�C�Y
	ULONG				m_nBlockArea ;			// �u���b�N�ʐ�
	ULONG				m_nBlockSamples ;		// �u���b�N�̃T���v����
	UINT				m_nChannelCount ;		// �`���l����
	ULONG				m_nWidthBlocks ;		// �摜�̕��i�u���b�N���j
	ULONG				m_nHeightBlocks ;		// �摜�̍����i�u���b�N���j

	UINT				m_nDstBytesPerPixel ;	// 1�s�N�Z���̃o�C�g��

	PBYTE				m_ptrOperations ;		// �I�y���[�V�����e�[�u��
	PINT				m_ptrColumnBuf ;		// ��o�b�t�@
	PINT				m_ptrLineBuf ;			// �s�o�b�t�@
	PINT				m_ptrBuffer ;			// �����o�b�t�@

public:
	typedef	void (ERIDecoder::*PFUNC_COLOR_OPRATION)( void ) ;
	typedef	void (ERIDecoder::*PFUNC_RESTORE)
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;

protected:
	PFUNC_COLOR_OPRATION	m_pfnColorOperation[0x10] ;

public:
	// �\�z�֐�
	ERIDecoder( void ) ;
	// ���Ŋ֐�
	virtual ~ERIDecoder( void ) ;

	// �������i�p�����[�^�̐ݒ�j
	int Initialize( const ERI_INFO_HEADER & infhdr ) ;
	// �I���i�������̉���Ȃǁj
	void Delete( void ) ;
	// �摜��W�J
	int DecodeImage
		( const RASTER_IMAGE_INFO & dstimginf,
			RLHDecodeContext & context, bool fTopDown ) ;

protected:
	// �t���J���[�摜�̓W�J
	int DecodeTrueColorImage
		( const RASTER_IMAGE_INFO & dstimginf, RLHDecodeContext & context ) ;
	// �p���b�g�摜�̓W�J
	int DecodePaletteImage
		( const RASTER_IMAGE_INFO & dstimginf, RLHDecodeContext & context ) ;

	// �W�J�摜���X�g�A����֐��ւ̃|�C���^���擾����
	virtual PFUNC_RESTORE GetRestoreFunc
		( DWORD fdwFormatType, DWORD dwBitsPerPixel ) ;

	// �J���[�I�y���[�V�����֐��Q
	void ColorOperation0000( void ) ;
	void ColorOperation0101( void ) ;
	void ColorOperation0110( void ) ;
	void ColorOperation0111( void ) ;
	void ColorOperation1001( void ) ;
	void ColorOperation1010( void ) ;
	void ColorOperation1011( void ) ;
	void ColorOperation1101( void ) ;
	void ColorOperation1110( void ) ;
	void ColorOperation1111( void ) ;

	// �O���C�摜�̓W�J
	void RestoreGray8
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGB�摜(15�r�b�g)�̓W�J
	void RestoreRGB16
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGB�摜�̓W�J
	void RestoreRGB24
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGBA�摜�̓W�J
	void RestoreRGBA32
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;

	// �W�J�i�s�󋵒ʒm�֐�
	virtual int OnDecodedBlock( LONG line, LONG column ) ;
	virtual int OnDecodedLine( LONG line ) ;

} ;


#endif
