
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


//
// 必要なデータ型やテキストマクロを定義します
//////////////////////////////////////////////////////////////////////////////
// コンパイルに必要なデータ型；
//		BYTE	: 符号無し8ビット整数
//		WORD	: 符号無し16ビット整数
//		DWORD	: 符号無し32ビット整数
//		SDWORD	: 符号有り32ビット整数
//		INT		: 符号有り16ビット以上整数
//		UINT	: 符号無し16ビット以上整数
//		LONG	: 符号有り32ビット以上整数
//		ULONG	: 符号無し32ビット以上整数
//		PVOID	: void 型へのポインタ
//		PBYTE	: 符号無し8ビット整数へのポインタ
//		PWORD	: 符号無し16ビット整数へのポインタ
//		PINT	: 符号有り16ビット以上整数へのポインタ

#include "eritypes.h"


//
// 標準恵理ちゃん展開ライブラリ定義ファイル
//////////////////////////////////////////////////////////////////////////////

#include "extrueri.h"


/*****************************************************************************
                   ランレングス・ガンマ・コンテキスト
 *****************************************************************************/

//
// 構築関数
//////////////////////////////////////////////////////////////////////////////
RLHDecodeContext::RLHDecodeContext( ULONG nBufferingSize )
{
	m_bytIntBufCount = 0 ;
	m_nBufferingSize = nBufferingSize ;
	m_nBufCount = 0 ;
	m_ptrBuffer = (PBYTE) ::eriAllocateMemory( nBufferingSize ) ;
	m_pStatisticalTable[0] = 0 ;
	m_pMTFTable[0] = 0 ;
	m_pfnDecodeSymbols = &RLHDecodeContext::DecodeGammaCodes ;
}

//
// 消滅関数
//////////////////////////////////////////////////////////////////////////////
RLHDecodeContext::~RLHDecodeContext( void )
{
	::eriFreeMemory( m_ptrBuffer ) ;
	if ( m_pStatisticalTable[0] != 0 )
		::eriFreeMemory( m_pStatisticalTable[0] ) ;
	if ( m_pMTFTable[0] != 0 )
		::eriFreeMemory( m_pMTFTable[0] ) ;
}

//
// 展開したデータを取得する（ガンマ符号）
//////////////////////////////////////////////////////////////////////////////
ULONG RLHDecodeContext::DecodeGammaCodes( PINT ptrDst, ULONG nCount )
{
	ULONG	nDecoded = 0, nRepeat ;
	INT		nSign, nCode ;
	//
	if ( m_nLength == 0 )
	{
		nCode = GetACode( ) ;
		if ( nCode == -1 )
			return	nDecoded ;
		m_nLength = (ULONG) nCode ;
	}
	//
	for ( ; ; )
	{
		//
		// 出力シンボル数を算出
		nRepeat = m_nLength ;
		if ( nRepeat > nCount )
		{
			nRepeat = nCount ;
		}
		m_nLength -= nRepeat ;
		nCount -= nRepeat ;
		//
		// シンボルを出力
		if ( !m_flgZero )
		{
			nDecoded += nRepeat ;
			while ( nRepeat -- )
				*(ptrDst ++) = 0 ;
		}
		else
		{
			while ( nRepeat -- )
			{
				nSign = GetABit( ) ;
				nCode = GetACode( ) ;
				if ( nCode == -1 )
					return	nDecoded ;
				nDecoded ++ ;
				*(ptrDst ++) = (nCode ^ nSign) - nSign ;
			}
		}
		//
		// 終了か？
		if ( nCount == 0 )
		{
			if ( m_nLength == 0 )
				m_flgZero = ~m_flgZero ;
			return	nDecoded ;
		}
		//
		// レングスコードを取得
		m_flgZero = ~m_flgZero ;
		nCode = GetACode( ) ;
		if ( nCode == -1 )
			return	nDecoded ;
		m_nLength = (ULONG) nCode ;
	}
}

//
// 算術符号の復号の準備する
//////////////////////////////////////////////////////////////////////////////
void RLHDecodeContext::PrepareToDecodeArithmeticCode( int nBitCount )
{
	//
	// 統計モデルを初期化
	//
	m_nSymbolBitCount = nBitCount ;
	m_nSymbolSortCount = (1 << nBitCount) ;
	m_maskSymbolBit = m_nSymbolSortCount - 1 ;
	//
	// メモリを確保
	m_pLastStatisticalModel = (STATISTICAL_MODEL*)
		::eriAllocateMemory( m_nSymbolSortCount *
			(sizeof(DWORD) + m_nSymbolSortCount * sizeof(ARITHCODE_SYMBOL)) ) ;
	//
	// 統計モデルを設定
	int	i, j ;
	int	maskSignBit = (m_nSymbolSortCount >> 1) ;
	int	bitSignExpansion = -1 << nBitCount ;
	m_pLastStatisticalModel->dwTotalSymbolCount = m_nSymbolSortCount ;
	ARITHCODE_SYMBOL *	pSymbolTable = m_pLastStatisticalModel->SymbolTable ;
	for ( i = 0; i < m_nSymbolSortCount; i ++ )
	{
		pSymbolTable[i].wOccured = 1 ;
		if ( i & maskSignBit )
			pSymbolTable[i].wSymbol = i | bitSignExpansion ;
		else
			pSymbolTable[i].wSymbol = i ;
	}
	//
	m_pStatisticalTable[0] = m_pLastStatisticalModel ;
	STATISTICAL_MODEL *	pStatisticalModel =
			(STATISTICAL_MODEL*)
				&(m_pLastStatisticalModel->SymbolTable[m_nSymbolSortCount]) ;
	//
	for ( i = 1; i < m_nSymbolSortCount; i ++ )
	{
		m_pStatisticalTable[i] = pStatisticalModel ;
		pStatisticalModel->dwTotalSymbolCount = m_nSymbolSortCount ;
		ARITHCODE_SYMBOL *	pacs = pStatisticalModel->SymbolTable ;
		//
		for ( j = 0; j < m_nSymbolSortCount; j ++ )
		{
			*(pacs ++) = pSymbolTable[j] ;
		}
		pStatisticalModel = (STATISTICAL_MODEL*) pacs ;
	}
	//
	// レジスタを初期化
	//
	m_dwCodeRegister = 0 ;
	m_dwAugendRegister = 0xFFFFFFFF ;
	m_dwCodeBuffer = GetNBits( 32 ) ;
	//
	for ( i = 0; i < 32; i ++ )
	{
		//
		// ビットスタッフィング
		if ( m_dwCodeBuffer == 0xFFFFFFFF )
		{
			m_dwCodeBuffer -= (GetABit() >> 1) ;
			if ( m_dwCodeBuffer == 0 )
				m_dwCodeRegister ++ ;
		}
		//
		// 1ビットシフトイン
		m_dwCodeRegister = (m_dwCodeRegister << 1) | (m_dwCodeBuffer >> 31) ;
		m_dwCodeBuffer = (m_dwCodeBuffer << 1) | (GetABit() & 0x01) ;
	}
	//
	// ビットスタッフィング
	if ( m_dwCodeBuffer == 0xFFFFFFFF )
	{
		m_dwCodeBuffer -= (GetABit() >> 1) ;
		if ( m_dwCodeBuffer == 0 )
			m_dwCodeRegister ++ ;
	}
	//
	// 展開関数設定
	m_nVirtualPostBuf = 0 ;
	m_pfnDecodeSymbols = &RLHDecodeContext::DecodeArithmeticCodes ;
}

//
// 展開したデータを取得する（算術符号）
//////////////////////////////////////////////////////////////////////////////
ULONG RLHDecodeContext::DecodeArithmeticCodes( PINT ptrDst, ULONG nCount )
{
	ULONG	nDecoded = 0 ;
	while ( nDecoded < nCount )
	{
		//
		// chop( C * N / A )
		DWORD	dwAcc = ::ChopMulDiv
			(
				m_dwCodeRegister,
				m_pLastStatisticalModel->dwTotalSymbolCount,
				m_dwAugendRegister
			) ;
		if ( dwAcc >= 0x10000 )
			break ;
		//
		// シンボルの指標を検索
		int	nSymbolIndex = 0 ;
		WORD	wAcc = (WORD) dwAcc ;
		WORD	wFs = 0 ;
		WORD	wOccured ;
		for ( ; ; )
		{
			wOccured =
				m_pLastStatisticalModel->SymbolTable[nSymbolIndex].wOccured ;
			if ( wAcc < wOccured )
				break ;
			wAcc -= wOccured ;
			wFs += wOccured ;
			nSymbolIndex ++ ;
			if ( nSymbolIndex >= m_nSymbolSortCount )
				return	nDecoded ;
		}
		//
		// コードレジスタとオージェンドレジスタを更新
		m_dwCodeRegister -= ::RaiseMulDiv
			(
				m_dwAugendRegister,
				wFs,
				m_pLastStatisticalModel->dwTotalSymbolCount
			) ;
		m_dwAugendRegister = ::ChopMulDiv
			(
				m_dwAugendRegister,
				wOccured,
				m_pLastStatisticalModel->dwTotalSymbolCount
			) ;
		//
		// オージェントレジスタを正規化し、コードレジスタに符号を読み込む
		while ( !(m_dwAugendRegister & 0x80000000) )
		{
			//
			// コードレジスタにシフトイン
			m_dwCodeRegister =
				(m_dwCodeRegister << 1) | (m_dwCodeBuffer >> 31) ;
			INT	nNextBit = GetABit( ) ;
			if ( nNextBit == 1 )
			{
				if ( (++ m_nVirtualPostBuf) >= 256 )
					return	nDecoded ;
				nNextBit = 0 ;
			}
			m_dwCodeBuffer =
				(m_dwCodeBuffer << 1) | (nNextBit & 0x01) ;
			//
			// ビットスタッフィング
			if ( m_dwCodeBuffer == 0xFFFFFFFF )
			{
				m_dwCodeBuffer -= (GetABit() >> 1) ;
				if ( m_dwCodeBuffer == 0 )
					m_dwCodeRegister ++ ;
			}
			//
			m_dwAugendRegister <<= 1 ;
		}
		//
		// 現在の統計モデルを更新
		ARITHCODE_SYMBOL	acsCurrent =
			m_pLastStatisticalModel->SymbolTable[nSymbolIndex] ;
		acsCurrent.wOccured ++ ;
		while ( nSymbolIndex != 0 )
		{
			ARITHCODE_SYMBOL	acs =
				m_pLastStatisticalModel->SymbolTable[nSymbolIndex - 1] ;
			if ( acs.wOccured > acsCurrent.wOccured )
				break ;
			m_pLastStatisticalModel->SymbolTable[nSymbolIndex] = acs ;
			nSymbolIndex -- ;
		}
		m_pLastStatisticalModel->SymbolTable[nSymbolIndex] = acsCurrent ;
		//
		// データを出力
		*(ptrDst ++) = acsCurrent.wSymbol ;
		//
		// 総合計生起シンボル数更新
		int	nSourceCode = (acsCurrent.wSymbol & m_maskSymbolBit) ;
		if ( (++ m_pLastStatisticalModel->dwTotalSymbolCount) >= 0x10000 )
		{
			DWORD	dwSymbolCount = 0 ;
			ARITHCODE_SYMBOL *	pacs =
				m_pLastStatisticalModel->SymbolTable ;
			for ( int i = 0; i < m_nSymbolSortCount; i ++ )
			{
				WORD	wOccured = pacs[i].wOccured ;
				wOccured = (wOccured & 0x01) + (wOccured >> 1) ;
				pacs[i].wOccured = wOccured ;
				dwSymbolCount += wOccured ;
			}
			m_pLastStatisticalModel->dwTotalSymbolCount = dwSymbolCount ;
		}
		//
		// 次のシンボルの復号へ
		m_pLastStatisticalModel = m_pStatisticalTable[nSourceCode] ;
		nDecoded ++ ;
	}
	return	nDecoded ;
}

//
// RL-MTF-G 符号の復号の準備をする
//////////////////////////////////////////////////////////////////////////////
void RLHDecodeContext::PrepareToDecodeRLMTFGCode( void )
{
	//
	// MTFテーブルを確保
	PBYTE	ptrBuffer =
		(PBYTE)::eriAllocateMemory( sizeof(MTF_TABLE) * 0x101 ) ;
	//
	// MTFテーブルを初期化
	for ( int i = 0; i <= 0x100; i ++ )
	{
		m_pMTFTable[i] = (MTF_TABLE*) ptrBuffer ;
		m_pMTFTable[i]->Initialize( ) ;
		ptrBuffer += sizeof(MTF_TABLE) ;
	}
}

//
// 1ライン展開したデータを取得する（RL-MTF-G 符号）
//////////////////////////////////////////////////////////////////////////////
ULONG RLHDecodeContext::DecodeRLMTFGCodes( PBYTE ptrDst, ULONG nCount )
{
	ULONG	nDecoded = 0 ;
	//
	// カレントMTFテーブルを設定
	int	iLastPlt = -1 ;
	MTF_TABLE *	pCurTable = m_pMTFTable[0x100] ;
	//
	// ラインを全てデコードするまで繰り返し
	while ( nDecoded < nCount )
	{
		//
		// ガンマ符号を復号
		INT		iCode = GetACode( ) - 1 ;
		if ( iCode & ~0xFF )
			break ;
		//
		// カレントMTFテーブルを参照する
		BYTE	iCurPlt = pCurTable->MoveToFront( iCode ) ;
		//
		// 復号されたパレット番号を出力
		ptrDst[nDecoded ++] = iCurPlt ;
		//
		// 直前のパレット番号と一致しないか比較
		if ( iLastPlt == (int) iCurPlt )
		{
			//
			// ランレングスの適用
			iCode = GetACode( ) - 1 ;
			if ( iCode < 0 )
				break ;
			if ( (ULONG) (nDecoded + iCode) > (ULONG) nCount )
				iCode = (INT) (nCount - nDecoded) ;
			//
			while ( iCode -- )
			{
				ptrDst[nDecoded ++] = iCurPlt ;
			}
		}
		else
		{
			//
			// カレントMTFテーブルの更新
			iLastPlt = iCurPlt ;
			pCurTable = m_pMTFTable[iCurPlt] ;
		}
	}
	//
	// 終了
	return	nDecoded ;
}


/*****************************************************************************
                               展開オブジェクト
 *****************************************************************************/

//
// 構築関数
//////////////////////////////////////////////////////////////////////////////
ERIDecoder::ERIDecoder( void )
{
	m_ptrOperations = 0 ;
	m_ptrColumnBuf = 0 ;
	m_ptrLineBuf = 0 ;
	m_ptrBuffer = 0 ;
	//
	m_pfnColorOperation[0]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[1]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[2]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[3]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[4]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[5]  = &ERIDecoder::ColorOperation0101 ;
	m_pfnColorOperation[6]  = &ERIDecoder::ColorOperation0110 ;
	m_pfnColorOperation[7]  = &ERIDecoder::ColorOperation0111 ;
	m_pfnColorOperation[8]  = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[9]  = &ERIDecoder::ColorOperation1001 ;
	m_pfnColorOperation[10] = &ERIDecoder::ColorOperation1010 ;
	m_pfnColorOperation[11] = &ERIDecoder::ColorOperation1011 ;
	m_pfnColorOperation[12] = &ERIDecoder::ColorOperation0000 ;
	m_pfnColorOperation[13] = &ERIDecoder::ColorOperation1101 ;
	m_pfnColorOperation[14] = &ERIDecoder::ColorOperation1110 ;
	m_pfnColorOperation[15] = &ERIDecoder::ColorOperation1111 ;
}

//
// 消滅関数
//////////////////////////////////////////////////////////////////////////////
ERIDecoder::~ERIDecoder( void )
{
	Delete( ) ;
}

//
// 初期化（パラメータの設定）
//////////////////////////////////////////////////////////////////////////////
int ERIDecoder::Initialize( const ERI_INFO_HEADER & infhdr )
{
	//
	// 以前のデータを消去
	Delete( ) ;
	//
	// 画像情報ヘッダをコピー
	m_EriInfHdr = infhdr ;
	//
	// パラメータのチェック
	if ( (m_EriInfHdr.fdwTransformation != 0x03020000)
		|| ((m_EriInfHdr.dwArchitecture != 0xFFFFFFFF)
			&& (m_EriInfHdr.dwArchitecture != 32)) )
	{
		return	1 ;		// エラー（未対応の圧縮フォーマット）
	}
	//
	switch ( (m_EriInfHdr.fdwFormatType & ERI_TYPE_MASK) )
	{
	case	ERI_RGB_IMAGE:
		if ( m_EriInfHdr.dwBitsPerPixel <= 8 )
			m_nChannelCount = 1 ;
		else if ( !(m_EriInfHdr.fdwFormatType & ERI_WITH_ALPHA) )
			m_nChannelCount = 3 ;
		else
			m_nChannelCount = 4 ;
		break ;

	case	ERI_GRAY_IMAGE:
		m_nChannelCount = 1 ;
		break;

	default:
		return	1 ;		// エラー（未対応の画像フォーマット）
	}
	//
	// 正常終了
	return	0 ;
}

//
// 終了（メモリの解放など）
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::Delete( void )
{
	if ( m_ptrOperations != 0 )
	{
		::eriFreeMemory( m_ptrOperations ) ;
		m_ptrOperations = 0 ;
	}
	if ( m_ptrColumnBuf != 0 )
	{
		::eriFreeMemory( m_ptrColumnBuf ) ;
		m_ptrColumnBuf = 0 ;
	}
	if ( m_ptrLineBuf != 0 )
	{
		::eriFreeMemory( m_ptrLineBuf ) ;
		m_ptrLineBuf = 0 ;
	}
	if ( m_ptrBuffer != 0 )
	{
		::eriFreeMemory( m_ptrBuffer ) ;
		m_ptrBuffer = 0 ;
	}
}

//
// 画像を展開
//////////////////////////////////////////////////////////////////////////////
int ERIDecoder::DecodeImage
( const RASTER_IMAGE_INFO & dstimginf,
		RLHDecodeContext & context, bool fTopDown )
{
	//
	// 出力画像情報をコピーする
	RASTER_IMAGE_INFO	imginf = dstimginf ;
	bool	fReverse = fTopDown ;
	if ( m_EriInfHdr.nImageHeight < 0 )
	{
		fReverse = ! fReverse ;
	}
	if ( fReverse )
	{
		imginf.ptrImageArray +=
			(imginf.nImageHeight - 1) * imginf.BytesPerLine ;
		imginf.BytesPerLine = - imginf.BytesPerLine ;
	}
	//
	// ERIバージョンを取得
	UINT	nERIVersion = context.GetNBits( 8 ) ;
	if ( nERIVersion == 1 )
	{
		//
		// フルカラーフォーマット
		return	DecodeTrueColorImage( imginf, context ) ;
	}
	else if ( nERIVersion == 2 )
	{
		//
		// パレット画像フォーマット
		return	DecodePaletteImage( imginf, context ) ;
	}
	else
	{
		return	1 ;			// 未対応のフォーマット
	}
}

//
// フルカラー画像の展開
//////////////////////////////////////////////////////////////////////////////
int ERIDecoder::DecodeTrueColorImage
( const RASTER_IMAGE_INFO & imginf, RLHDecodeContext & context )
{
	//
	// 各定数を計算
	m_nBlockSize = (ULONG) (1 << m_EriInfHdr.dwBlockingDegree) ;
	m_nBlockArea = (ULONG) (1 << (m_EriInfHdr.dwBlockingDegree << 1)) ;
	m_nBlockSamples = m_nBlockArea * m_nChannelCount ;
	m_nWidthBlocks =
		(ULONG) ((m_EriInfHdr.nImageWidth + m_nBlockSize - 1)
							>> m_EriInfHdr.dwBlockingDegree) ;
	if ( m_EriInfHdr.nImageHeight < 0 )
	{
		m_nHeightBlocks = (ULONG) - m_EriInfHdr.nImageHeight ;
	}
	else
	{
		m_nHeightBlocks = (ULONG) m_EriInfHdr.nImageHeight ;
	}
	m_nHeightBlocks =
		(m_nHeightBlocks + m_nBlockSize - 1)
							>> m_EriInfHdr.dwBlockingDegree ;
	//
	// ワーキングメモリを確保
	m_ptrOperations =
		(PBYTE) ::eriAllocateMemory( m_nWidthBlocks * m_nHeightBlocks ) ;
	m_ptrColumnBuf =
		(PINT) ::eriAllocateMemory
			( m_nBlockSize * m_nChannelCount * sizeof(INT) ) ;
	m_ptrLineBuf =
		(PINT) ::eriAllocateMemory( m_nChannelCount *
			(m_nWidthBlocks << m_EriInfHdr.dwBlockingDegree) * sizeof(INT) ) ;
	m_ptrBuffer =
		(PINT) ::eriAllocateMemory( m_nBlockSamples * sizeof(INT) ) ;
	//
	// 画像ストア関数のアドレスを取得
	PFUNC_RESTORE	pfnRestore =
		GetRestoreFunc( imginf.fdwFormatType, imginf.dwBitsPerPixel ) ;
	if ( pfnRestore == 0 )
		return	1 ;			// エラー（該当するストア関数が定義されていない）
	//
	// ラインバッファをクリア
	INT		i, j, k ;
	LONG	nWidthSamples =
		m_nChannelCount * (m_nWidthBlocks << m_EriInfHdr.dwBlockingDegree) ;
	for ( i = 0; i < nWidthSamples; i ++ )
	{
		m_ptrLineBuf[i] = 0 ;
	}
	//
	// 展開開始
	//
	// ERIヘッダを取得
	UINT	fOpTable = context.GetNBits( 8 ) ;
	UINT	fEncodeType = context.GetNBits( 8 ) ;
	UINT	fReserved = context.GetNBits( 8 ) ;
	int		nBitCount = 0 ;
	if ( m_EriInfHdr.dwArchitecture == 32 )
	{
		if ( (fOpTable != 0) || (fReserved != 0) )
		{
			return	1 ;					// 未対応のフォーマット
		}
		nBitCount = fEncodeType ;
		fEncodeType = 1 ;
	}
	else
	{
		if ( (fOpTable != 0)
			|| (fEncodeType & 0xFE) || (fReserved != 0) )
		{
			return	1 ;					// 未対応のフォーマット
		}
	}
	//
	// オペレーションテーブルを取得
	PBYTE	pNextOperation = m_ptrOperations ;
	if ( (fEncodeType & 0x01) && (m_nChannelCount >= 3) )
	{
		LONG	nAllBlockCount = (LONG) (m_nWidthBlocks * m_nHeightBlocks) ;
		for ( i = 0; i < nAllBlockCount; i ++ )
		{
			m_ptrOperations[i] = (BYTE) context.GetNBits( 4 ) ;
		}
	}
	//
	// コンテキストを初期化
	if ( context.GetABit( ) != 0 )
	{
		return	1 ;					// 不正なフォーマット
	}
	if ( m_EriInfHdr.dwArchitecture == 32 )
	{
		context.PrepareToDecodeArithmeticCode( nBitCount ) ;
	}
	else
	{
		if ( fEncodeType & 0x01 )
		{
			context.Initialize( ) ;
		}
	}
	//
	LONG	nPosX, nPosY ;
	LONG	nAllBlockLines = (LONG) (m_nBlockSize * m_nChannelCount) ;
	LONG	nRestHeight = (LONG) imginf.nImageHeight ;
	//
	for ( nPosY = 0; nPosY < (LONG) m_nHeightBlocks; nPosY ++ )
	{
		//
		// カラムバッファをクリア
		LONG	nColumnBufSamples = (LONG) (m_nBlockSize * m_nChannelCount) ;
		for ( i = 0; i < nColumnBufSamples; i ++ )
		{
			m_ptrColumnBuf[i] = 0 ;
		}
		//
		// 行の事前処理
		PBYTE	ptrDstLine = imginf.ptrImageArray
			+ ((nPosY * imginf.BytesPerLine) << m_EriInfHdr.dwBlockingDegree) ;
		int	nBlockHeight = (int) m_nBlockSize ;
		if ( nBlockHeight > nRestHeight )
			nBlockHeight = nRestHeight ;
		//
		LONG	nRestWidth = (LONG) imginf.nImageWidth ;
		PINT	ptrNextLineBuf = m_ptrLineBuf ;
		//
		for ( nPosX = 0; nPosX < (LONG) m_nWidthBlocks; nPosX ++ )
		{
			//
			// ランレングス・ガンマをデコード
			UINT	nColorOperation ;
			if ( !(fEncodeType & 0x01) )
			{
				nColorOperation = context.GetNBits( 4 ) ;
				context.Initialize( ) ;
			}
			else
			{
				nColorOperation = *(pNextOperation ++) ;
			}
			if ( context.DecodeSymbols
				( m_ptrBuffer, m_nBlockSamples ) < m_nBlockSamples )
			{
				return	1 ;			// エラー（デコードに失敗）
			}
			//
			// カラーオペレーションを実行
			if ( m_nChannelCount >= 3 )
			{
				(this->*m_pfnColorOperation[nColorOperation])( ) ;
			}
			//
			// 差分処理を実行（水平方向）
			PINT	ptrNextBuf = m_ptrBuffer ;
			PINT	ptrNextColBuf = m_ptrColumnBuf ;
			for ( i = 0; i < nAllBlockLines; i ++ )
			{
				INT	nLastVal = *ptrNextColBuf ;
				for ( j = 0; j < (INT) m_nBlockSize; j ++ )
				{
					nLastVal += *ptrNextBuf ;
					*(ptrNextBuf ++) = nLastVal ;
				}
				*(ptrNextColBuf ++) = nLastVal ;
			}
			//
			// 差分処理を実行（垂直方向）
			ptrNextBuf = m_ptrBuffer ;
			for ( k = 0; k < (INT) m_nChannelCount; k ++ )
			{
				PINT	ptrLastLine = ptrNextLineBuf ;
				for ( i = 0; i < (INT) m_nBlockSize; i ++ )
				{
					PINT	ptrCurrentLine = ptrNextBuf ;
					for ( j = 0; j < (INT) m_nBlockSize; j ++ )
					{
						*(ptrNextBuf ++) += *(ptrLastLine ++) ;
					}
					ptrLastLine = ptrCurrentLine ;
				}
				for ( j = 0; j < (INT) m_nBlockSize; j ++ )
				{
					*(ptrNextLineBuf ++) = *(ptrLastLine ++) ;
				}
			}
			//
			// 処理結果を出力バッファにストア
			int	nBlockWidth = (int) m_nBlockSize ;
			if ( nBlockWidth > nRestWidth )
				nBlockWidth = nRestWidth ;
			(this->*pfnRestore)
				( ptrDstLine, imginf.BytesPerLine, nBlockWidth, nBlockHeight ) ;
			//
			// 展開の状況を通知
			int	flgContinue = OnDecodedBlock( nPosY, nPosX ) ;
			if ( flgContinue != 0 )
				return	flgContinue ;	// 中断
			//
			ptrDstLine +=
				(m_nDstBytesPerPixel << m_EriInfHdr.dwBlockingDegree) ;
			nRestWidth -= (LONG) m_nBlockSize ;
		}
		//
		nRestHeight -= (LONG) m_nBlockSize ;
	}
	//
	return	0 ;				// 正常終了
}

//
// パレット画像の展開
//////////////////////////////////////////////////////////////////////////////
int ERIDecoder::DecodePaletteImage
( const RASTER_IMAGE_INFO & imginf, RLHDecodeContext & context )
{
	//
	// フォーマットの確認
	if ( imginf.dwBitsPerPixel != 8 )
		return	1 ;			// エラー（未対応のフォーマット）
	//
	// ERIヘッダの確認
	UINT	fReserved = context.GetNBits( 24 ) ;
	if ( fReserved != 0 )
		return	1 ;			// エラー（未対応のフォーマット）
	//
	// コンテキストの初期化
	if ( m_EriInfHdr.dwArchitecture == 32 )
	{
		context.PrepareToDecodeArithmeticCode( 8 ) ;
		m_ptrBuffer =
			(PINT) ::eriAllocateMemory( imginf.nImageWidth * sizeof(INT) ) ;
	}
	else
	{
		context.PrepareToDecodeRLMTFGCode( ) ;
	}
	//
	// 復号開始
	LONG	nLine, nHeight ;
	nHeight = imginf.nImageHeight ;
	PBYTE	ptrDstLine = imginf.ptrImageArray ;
	ULONG	nImageWidth = (ULONG) imginf.nImageWidth ;
	SDWORD	nBytesPerLine = imginf.BytesPerLine ;
	//
	for ( nLine = 0; nLine < nHeight; nLine ++ )
	{
		if ( m_EriInfHdr.dwArchitecture == 32 )
		{
			//
			// 算術符号を使って1行展開
			if ( context.DecodeSymbols
					( m_ptrBuffer, nImageWidth ) < nImageWidth )
			{
				return	1 ;				// エラー（デコードに失敗）
			}
			//
			for ( ULONG i = 0; i < nImageWidth; i ++ )
			{
				ptrDstLine[i] = (BYTE) m_ptrBuffer[i] ;
			}
		}
		else
		{
			//
			// RL-MTF-G 符号を使って1行展開
			if ( context.DecodeRLMTFGCodes
					( ptrDstLine, nImageWidth ) < nImageWidth )
			{
				return	1 ;				// エラー（デコードに失敗）
			}
		}
		//
		// 展開の状況を通知
		int	flgContinue = OnDecodedLine( nLine ) ;
		if ( flgContinue != 0 )
			return	flgContinue ;	// 中断
		//
		ptrDstLine += nBytesPerLine ;
	}
	//
	return	0 ;				// 正常終了
}

//
// 展開画像をストアする関数へのポインタを取得する
//////////////////////////////////////////////////////////////////////////////
ERIDecoder::PFUNC_RESTORE
	ERIDecoder::GetRestoreFunc( DWORD fdwFormatType, DWORD dwBitsPerPixel )
{
	m_nDstBytesPerPixel = (UINT) dwBitsPerPixel >> 3 ;

	switch ( m_EriInfHdr.fdwFormatType )
	{
	case	ERI_RGB_IMAGE:
		if ( dwBitsPerPixel == 16 )
		{
			return	&ERIDecoder::RestoreRGB16 ;
		}
		else if ( (dwBitsPerPixel == 24)
					|| (dwBitsPerPixel == 32) )
		{
			return	&ERIDecoder::RestoreRGB24 ;
		}
		break ;

	case	ERI_RGBA_IMAGE:
		if ( dwBitsPerPixel == 32 )
			return	&ERIDecoder::RestoreRGBA32 ;
		break ;

	case	ERI_GRAY_IMAGE:
		if ( dwBitsPerPixel == 8 )
			return	&ERIDecoder::RestoreGray8 ;
		break ;
	}
	return	0 ;
}

//
// カラーオペレーション関数群
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::ColorOperation0000( void )
{
}

void ERIDecoder::ColorOperation0101( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = *ptrNext ;
		ptrNext[nChannelSamples] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation0110( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = *ptrNext ;
		ptrNext[nChannelSamples * 2] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation0111( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = *ptrNext ;
		ptrNext[nChannelSamples] += nBase ;
		ptrNext[nChannelSamples * 2] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1001( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples] ;
		*ptrNext += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1010( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples] ;
		ptrNext[nChannelSamples * 2] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1011( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples] ;
		*ptrNext += nBase ;
		ptrNext[nChannelSamples * 2] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1101( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples * 2] ;
		*ptrNext += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1110( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples * 2] ;
		ptrNext[nChannelSamples] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

void ERIDecoder::ColorOperation1111( void )
{
	LONG	nChannelSamples = m_nBlockArea ;
	PINT	ptrNext = m_ptrBuffer ;
	LONG	nRepCount = m_nBlockArea ;
	//
	do
	{
		INT	nBase = ptrNext[nChannelSamples * 2] ;
		*ptrNext += nBase ;
		ptrNext[nChannelSamples] += nBase ;
		ptrNext ++ ;
	}
	while ( -- nRepCount ) ;
}

//
// グレイ画像の展開
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::RestoreGray8
	( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight )
{
	PINT	ptrNextBuf = m_ptrBuffer ;
	//
	while ( nHeight -- )
	{
		for ( int x = 0; x < nWidth; x ++ )
		{
			ptrDst[x] = (BYTE) ptrNextBuf[x] ;
		}
		ptrNextBuf += m_nBlockSize ;
		ptrDst += nLineBytes ;
	}
}

//
// RGB画像(15ビット)の展開
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::RestoreRGB16
	( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight )
{
	PBYTE	ptrDstLine = ptrDst ;
	PINT	ptrBufLine = m_ptrBuffer ;
	LONG	nBlockSamples = (LONG) m_nBlockArea ;
	//
	while ( nHeight -- )
	{
		PINT	ptrNextBuf = ptrBufLine ;
		PWORD	pwDst = (PWORD) ptrDstLine ;
		//
		int	i = nWidth ;
		while ( i -- )
		{
			*pwDst = (WORD)
				(((int) *ptrNextBuf & 0x1F)) |
				(((int) ptrNextBuf[nBlockSamples] & 0x1F) << 5) |
				(((int) ptrNextBuf[nBlockSamples * 2] & 0x1F) << 10) ;
			ptrNextBuf ++ ;
			pwDst ++ ;
		}
		ptrBufLine += m_nBlockSize ;
		ptrDstLine += nLineBytes ;
	}
}

//
// RGB画像の展開
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::RestoreRGB24
	( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight )
{
	PBYTE	ptrDstLine = ptrDst ;
	PINT	ptrBufLine = m_ptrBuffer ;
	int		nBytesPerPixel = (int) m_nDstBytesPerPixel ;
	LONG	nBlockSamples = (LONG) m_nBlockArea ;
	//
	while ( nHeight -- )
	{
		PINT	ptrNextBuf = ptrBufLine ;
		ptrDst = ptrDstLine ;
		//
		int	i = nWidth ;
		while ( i -- )
		{
			ptrDst[0] = (BYTE) *ptrNextBuf ;
			ptrDst[1] = (BYTE) ptrNextBuf[nBlockSamples] ;
			ptrDst[2] = (BYTE) ptrNextBuf[nBlockSamples * 2] ;
			ptrNextBuf ++ ;
			ptrDst += nBytesPerPixel ;
		}
		ptrBufLine += m_nBlockSize ;
		ptrDstLine += nLineBytes ;
	}
}

//
// RGBA画像の展開
//////////////////////////////////////////////////////////////////////////////
void ERIDecoder::RestoreRGBA32
	( PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight )
{
	PBYTE	ptrDstLine = ptrDst ;
	PINT	ptrBufLine = m_ptrBuffer ;
	LONG	nBlockSamples = (LONG) m_nBlockArea ;
	LONG	nBlockSamplesX3 = nBlockSamples * 3 ;
	//
	while ( nHeight -- )
	{
		PINT	ptrNextBuf = ptrBufLine ;
		ptrDst = ptrDstLine ;
		//
		int	i = nWidth ;
		while ( i -- )
		{
			ptrDst[0] = (BYTE) *ptrNextBuf ;
			ptrDst[1] = (BYTE) ptrNextBuf[nBlockSamples] ;
			ptrDst[2] = (BYTE) ptrNextBuf[nBlockSamples * 2] ;
			ptrDst[3] = (BYTE) ptrNextBuf[nBlockSamplesX3] ;
			ptrNextBuf ++ ;
			ptrDst += 4 ;
		}
		ptrBufLine += m_nBlockSize ;
		ptrDstLine += nLineBytes ;
	}
}

//
// 展開進行状況通知関数
//////////////////////////////////////////////////////////////////////////////
int ERIDecoder::OnDecodedBlock( LONG line, LONG column )
{
	return	0 ;			// 処理を継続
}

int ERIDecoder::OnDecodedLine( LONG line )
{
	return	0 ;			// 処理を継続
}

