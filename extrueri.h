
/*****************************************************************************
                       標準恵理ちゃん展開ライブラリ
                                                      最終更新 2000/09/14
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
                         メモリアロケーション
 *****************************************************************************/

extern	PVOID eriAllocateMemory( DWORD dwBytes ) ;
extern	void eriFreeMemory( PVOID pMemory ) ;


/*****************************************************************************
                             演算関数
 *****************************************************************************/

extern	DWORD ChopMulDiv( DWORD x, DWORD y, DWORD z ) ;
extern	DWORD RaiseMulDiv( DWORD x, DWORD y, DWORD z ) ;


/*****************************************************************************
                                画像情報
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
                   ランレングス・ガンマ・コンテキスト
 *****************************************************************************/

class	RLHDecodeContext
{
protected:
	int		m_flgZero ;			// ゼロフラグ
	ULONG	m_nLength ;			// ランレングス
	BYTE	m_bytIntBufCount ;	// 中間入力バッファに蓄積されているビット数
	BYTE	m_bytIntBuffer ;	// 中間入力バッファ
	ULONG	m_nBufferingSize ;	// バッファリングするバイト数
	ULONG	m_nBufCount ;		// バッファの残りバイト数
	PBYTE	m_ptrBuffer ;		// 入力バッファの先頭へのポインタ
	PBYTE	m_ptrNextBuf ;		// 次に読み込むべき入力バッファへのポインタ

	struct	ARITHCODE_SYMBOL
	{
		WORD	wOccured ;					// シンボルの出現回数
		SWORD	wSymbol ;					// シンボル
	} ;
	struct	STATISTICAL_MODEL
	{
		DWORD				dwTotalSymbolCount ;	// 全シンボル数 < 10000H
		ARITHCODE_SYMBOL	SymbolTable[1] ;		// 統計モデル
	} ;
	DWORD	m_dwCodeRegister ;		// コードレジスタ
	DWORD	m_dwAugendRegister ;	// オージェンドレジスタ
	DWORD	m_dwCodeBuffer ;		// ビットバッファ
	int		m_nVirtualPostBuf ;		// 仮想バッファカウンタ
	int		m_nSymbolBitCount ;		// 符号のビット数
	int		m_nSymbolSortCount ;	// 符号の総数
	int		m_maskSymbolBit ;		// 符号のビットマスク
	STATISTICAL_MODEL *	m_pLastStatisticalModel ;		// 最後の統計モデル
	STATISTICAL_MODEL *	m_pStatisticalTable[0x100] ;	// 統計モデル

	struct	MTF_TABLE
	{
		BYTE	iplt[0x100] ;

		inline void Initialize( void ) ;
		inline BYTE MoveToFront( int index ) ;
	} ;
	MTF_TABLE *	m_pMTFTable[0x101] ;	// MTFテーブル

	typedef	ULONG (RLHDecodeContext::*PFUNC_DECODE_SYMBOLS)
									( PINT ptrDst, ULONG nCount ) ;
	PFUNC_DECODE_SYMBOLS	m_pfnDecodeSymbols ;

public:
	// 構築関数
	RLHDecodeContext( ULONG nBufferingSize ) ;
	// 消滅関数
	virtual ~RLHDecodeContext( void ) ;

	// ゼロフラグを読み込んで、コンテキストを初期化する
	inline void Initialize( void ) ;
	// バッファが空の時、次のデータを読み込む
	inline int PrefetchBuffer( void ) ;
	// 1ビット取得する（ 0 又は－1を返す ）
	inline INT GetABit( void ) ;
	// nビット取得する
	inline UINT GetNBits( int n ) ;
	// ガンマコードを取得する
	inline INT GetACode( void ) ;
	// 次のデータを読み込む
	virtual ULONG ReadNextData( PBYTE ptrBuffer, ULONG nBytes ) = 0 ;

	// 算術符号の復号の準備する
	void PrepareToDecodeArithmeticCode( int nBitCount ) ;
	// RL-MTF-G 符号の復号の準備をする
	void PrepareToDecodeRLMTFGCode( void ) ;

	// 展開したデータを取得する
	inline ULONG DecodeSymbols( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeGammaCodes( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeArithmeticCodes( PINT ptrDst, ULONG nCount ) ;
	ULONG DecodeRLMTFGCodes( PBYTE ptrDst, ULONG nCount ) ;

} ;

//
// MTF_TABLE を初期化する
//////////////////////////////////////////////////////////////////////////////
inline void RLHDecodeContext::MTF_TABLE::Initialize( void )
{
	for ( int i = 0; i < 0x100; i ++ )
		iplt[i] = (BYTE) i ;
}

//
// Move To Front を実行
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
// ゼロフラグを読み込んで、コンテキストを初期化する
//////////////////////////////////////////////////////////////////////////////
inline void RLHDecodeContext::Initialize( void )
{
	m_flgZero = (int) GetABit( ) ;
	m_nLength = 0 ;
}

//
// バッファが空の時、次のデータを読み込む
//////////////////////////////////////////////////////////////////////////////
// 返り値；
//		0の時、正常終了
//		1の時、異常終了
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
// 1ビットを取得する
//////////////////////////////////////////////////////////////////////////////
// 返り値；
//		0 又は－1を返す。
//		エラーが発生した場合は1を返す。
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
// nビット取得する
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
// ガンマコードを取得する
//////////////////////////////////////////////////////////////////////////////
// 返り値；
//		取得されたガンマコードを返す。
//		エラーが発生した場合は－1を返す。
//////////////////////////////////////////////////////////////////////////////
inline INT RLHDecodeContext::GetACode( void )
{
	INT	nCode = 0, nBase = 1 ;
	//
	for ( ; ; )
	{
		//
		// バッファへのデータの読み込み
		if ( PrefetchBuffer( ) )
			return	(LONG) -1 ;
		//
		// 1ビット取り出し
		m_bytIntBufCount -- ;
		if ( !(m_bytIntBuffer & 0x80) )
		{
			//
			// コード終了
			nCode += nBase ;
			m_bytIntBuffer <<= 1 ;
			return	nCode ;
		}
		else
		{
			//
			// コード継続
			nBase <<= 1 ;
			m_bytIntBuffer <<= 1 ;
			//
			// バッファへのデータの読み込み
			if ( PrefetchBuffer( ) )
				return	(LONG) -1 ;
			//
			// 1ビット取り出し
			nCode = (nCode << 1) | (m_bytIntBuffer >> 7) ;
			m_bytIntBufCount -- ;
			m_bytIntBuffer <<= 1 ;
		}
	}
}

//
// 展開したデータを取得する
//////////////////////////////////////////////////////////////////////////////
inline ULONG RLHDecodeContext::DecodeSymbols( PINT ptrDst, ULONG nCount )
{
	return	(this->*m_pfnDecodeSymbols)( ptrDst, nCount ) ;
}


/*****************************************************************************
                               展開オブジェクト
 *****************************************************************************/

class	ERIDecoder
{
protected:
	ERI_INFO_HEADER		m_EriInfHdr ;			// 画像情報ヘッダ

	UINT				m_nBlockSize ;			// ブロッキングサイズ
	ULONG				m_nBlockArea ;			// ブロック面積
	ULONG				m_nBlockSamples ;		// ブロックのサンプル数
	UINT				m_nChannelCount ;		// チャネル数
	ULONG				m_nWidthBlocks ;		// 画像の幅（ブロック数）
	ULONG				m_nHeightBlocks ;		// 画像の高さ（ブロック数）

	UINT				m_nDstBytesPerPixel ;	// 1ピクセルのバイト数

	PBYTE				m_ptrOperations ;		// オペレーションテーブル
	PINT				m_ptrColumnBuf ;		// 列バッファ
	PINT				m_ptrLineBuf ;			// 行バッファ
	PINT				m_ptrBuffer ;			// 内部バッファ

public:
	typedef	void (ERIDecoder::*PFUNC_COLOR_OPRATION)( void ) ;
	typedef	void (ERIDecoder::*PFUNC_RESTORE)
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;

protected:
	PFUNC_COLOR_OPRATION	m_pfnColorOperation[0x10] ;

public:
	// 構築関数
	ERIDecoder( void ) ;
	// 消滅関数
	virtual ~ERIDecoder( void ) ;

	// 初期化（パラメータの設定）
	int Initialize( const ERI_INFO_HEADER & infhdr ) ;
	// 終了（メモリの解放など）
	void Delete( void ) ;
	// 画像を展開
	int DecodeImage
		( const RASTER_IMAGE_INFO & dstimginf,
			RLHDecodeContext & context, bool fTopDown ) ;

protected:
	// フルカラー画像の展開
	int DecodeTrueColorImage
		( const RASTER_IMAGE_INFO & dstimginf, RLHDecodeContext & context ) ;
	// パレット画像の展開
	int DecodePaletteImage
		( const RASTER_IMAGE_INFO & dstimginf, RLHDecodeContext & context ) ;

	// 展開画像をストアする関数へのポインタを取得する
	virtual PFUNC_RESTORE GetRestoreFunc
		( DWORD fdwFormatType, DWORD dwBitsPerPixel ) ;

	// カラーオペレーション関数群
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

	// グレイ画像の展開
	void RestoreGray8
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGB画像(15ビット)の展開
	void RestoreRGB16
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGB画像の展開
	void RestoreRGB24
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;
	// RGBA画像の展開
	void RestoreRGBA32
		( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight ) ;

	// 展開進行状況通知関数
	virtual int OnDecodedBlock( LONG line, LONG column ) ;
	virtual int OnDecodedLine( LONG line ) ;

} ;


#endif
