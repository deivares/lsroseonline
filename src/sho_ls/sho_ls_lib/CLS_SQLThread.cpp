
#include "stdAFX.h"
#include "blockLIST.h"

#include "CLS_Account.h"
#include "CLS_Server.h"
#include "CLS_Client.h"
#include "CLS_SqlTHREAD.h"
#include "CAS_GUMS.h"

extern classListBLOCK<tagBlockDATA> *g_pListBlackACCOUNT;

IMPLEMENT_INSTANCE( CLS_SqlTHREAD )

//-------------------------------------------------------------------------------------------------
CLS_SqlTHREAD::CLS_SqlTHREAD () : CSqlTHREAD( true )
{
    m_bCheckLogIN = false;
    m_dwCheckRIGHT = 0;

	::ZeroMemory( m_szTmpMD5, sizeof(m_szTmpMD5) );
}
__fastcall CLS_SqlTHREAD::~CLS_SqlTHREAD ()
{
    this->Free ();
}

//-------------------------------------------------------------------------------------------------
bool CLS_SqlTHREAD::Add_SqlPACKET (DWORD dwSocketID, char *szAccount, t_PACKET *pPacket)
{
    if ( 0 == dwSocketID )
        return false;

    // ���� �Ұ� ���� ���� !!!
    char *szID;
    short nOffset=sizeof( cli_LOGIN_REQ );
    szID = Packet_GetStringPtr( pPacket, nOffset );

    t_HASHKEY HashKEY = CStr::GetHASH( szID );
    tagBlockDATA *pBlockID = g_pListBlackACCOUNT->Search( HashKEY, szID, true );
    if ( pBlockID ) {
        g_pListBlackACCOUNT->Update( pBlockID, 0, 0 );
        return false;
    }

	return CSqlTHREAD::Add_SqlPACKET( dwSocketID, szAccount, (BYTE*)pPacket, pPacket->m_HEADER.m_nSize);
}

bool CLS_SqlTHREAD::Add_LogOutUSER (CLS_Account *pCAccount)
{
	m_csUserLIST.Lock ();
		m_AddUserLIST.AppendNode( pCAccount->m_pListNODE );
	m_csUserLIST.Unlock ();

	m_pEVENT->SetEvent ();

	return true;
}

//-------------------------------------------------------------------------------------------------
void CLS_SqlTHREAD::Execute ()
{
	// ��������...�α��� ������..�ٸ� ó���� ���ؼ�...����...
	// this->SetPriority( THREAD_PRIORITY_HIGHEST );	// Priority 2 point above the priority class
	// this->SetPriority( THREAD_PRIORITY_ABOVE_NORMAL );	// Priority 2 point above the priority class

	CDLList< tagQueryDATA >::tagNODE *pSqlNODE;
	classDLLNODE< CLS_Account*  > *pUsrNODE;

	g_LOG.CS_ODS( 0xffff, ">>>> CLS_SqlTHREAD::Execute() ThreadID: %d(0x%x)\n", this->ThreadID, this->ThreadID );

    while( TRUE ) {
		if ( !this->Terminated ) {
			m_pEVENT->WaitFor( INFINITE );
		} else {
			int iReaminCNT;
			this->m_CS.Lock ();
				iReaminCNT = m_AddPACKET.GetNodeCount();
			this->m_CS.Unlock ();

			if ( iReaminCNT <= 0 )
				break;
		}

		this->m_CS.Lock ();
			m_RunPACKET.AppendNodeList( &m_AddPACKET );
			m_AddPACKET.Init ();
		this->m_CS.Unlock ();
		m_pEVENT->ResetEvent ();

		for( pSqlNODE = m_RunPACKET.GetHeadNode(); pSqlNODE; ) {
			if ( Run_SqlPACKET( &pSqlNODE->m_VALUE ) )
				pSqlNODE = this->Del_SqlPACKET( pSqlNODE );
			else
				pSqlNODE = m_RunPACKET.GetNextNode ( pSqlNODE );
		}

		m_csUserLIST.Lock ();
			m_RunUserLIST.AppendNodeList( &m_AddUserLIST );
			m_AddUserLIST.Init ();
		m_csUserLIST.Unlock ();

        for (pUsrNODE=m_RunUserLIST.GetHeadNode(); pUsrNODE; pUsrNODE=m_RunUserLIST.GetHeadNode() ) {
			m_RunUserLIST.DeleteNode( pUsrNODE->DATA->m_pListNODE );
			// ��� CLS_Account()�� ���⼭ �����ȴ�...
            g_pListJOIN->Delete_ACCOUNT( pUsrNODE->DATA );
		}
	}

	g_LOG.CS_ODS( 0xffff, "<<<< CLS_SqlTHREAD::Execute() ThreadID: %d(0x%x)\n", this->ThreadID, this->ThreadID );
}

//-------------------------------------------------------------------------------------------------
bool CLS_SqlTHREAD::Run_SqlPACKET( tagQueryDATA *pSqlPACKET )
{
	switch( pSqlPACKET->m_pPacket->m_wType ) {
		case CLI_LOGIN_REQ :
			if ( !Proc_cli_LOGIN_REQ( pSqlPACKET ) )  {
				CLS_Client *pClient = (CLS_Client *)g_pListCLIENT->GetSOCKET( pSqlPACKET->m_iTAG );
				if ( pClient )
					pClient->m_nProcSTEP = CLIENT_STEP_LOGIN_WAIT;
				return true;
			}
			break;

		case BKD_SERVER_MAKEACCO :
		{
			if( 1 )
			{
			}
			break;
		}

		case BKD_SERVER_MODACCO :
		{
			break;
		}
/*
        case GSV_LOGOUT :
            Proc_gsv_LOGOUT( pSqlPACKET );
            break;
*/
		default :
			g_LOG.CS_ODS( 0xffff, "Undefined sql packet Type: %x, Size: %d \n", pSqlPACKET->m_pPacket->m_wType, pSqlPACKET->m_pPacket->m_nSize);
	}

	return true;
}

enum LOGINTBL_COL_IDX {
		LGNTBL_RIGHT,
		LGNTBL_PASSWORD,
		LGNTBL_LAST_CONNECT,
#ifdef	USE_ORACLE_DB
		LGNTBL_BLOCK_START,
		LGNTBL_BLOCK_END,
		LGNTBL_GENDER,
	#ifndef		USE_MSSQL
		LGNTBL_JUMIN,
	#endif
		LGNTBL_REALNAME,
#else
		LGNTBL_ENABLE,
#endif
} ;

//-------------------------------------------------------------------------------------------------
#define	MAX_ACCOUNT_LEN		40		// �ִ� �����Է� ���� 16
bool CLS_SqlTHREAD::Proc_cli_LOGIN_REQ( tagQueryDATA *pSqlPACKET )
{
	t_PACKET *pPacket = (t_PACKET*)pSqlPACKET->m_pPacket;

    char *szAccount;
    short nOffset=sizeof( cli_LOGIN_REQ ), nOutStrLen;

    szAccount = Packet_GetStringPtr( pPacket, nOffset, nOutStrLen );
	
	if ( NULL == szAccount || nOutStrLen > MAX_ACCOUNT_LEN ) {
		// Ŭ���̾�Ʈ ���׷� �Էµ� ������ ���̰� �ʰ��Ǽ� ��.
		g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_NOT_FOUND_ACCOUNT );
		return false;
	}

	DWORD dwRIGHT = 0;

#ifdef	USE_ORACLE_DB
	#ifdef	USE_MSSQL
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MS-SQL �ؿ� ����~~~~~")
//	if ( !this->m_pSQL->QuerySQL ( "SELECT [right],md5password,lastconnect,blockstart,blockend,gender,jumin FROM userinfo WHERE UPPER(account)=UPPER(\'%s\')", szAccount ) )
	if ( !this->m_pSQL->QuerySQL( "{call UserAuthenticate(\'%s\')}", szAccount ) )	// SQL LOGIN
	#else
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ ORACLE ���� ����~~~~~")
	if ( !this->m_pSQL->QuerySQL ( "SELECT right,md5password,lastconnect,blockstart,blockend,gender,jumin, realname FROM userinfo WHERE UPPER(account)=UPPER(\'%s\')", szAccount ) )
	#endif
#else
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
		#pragma COMPILE_TIME_MSG("            @@@@@@@@@@@@ MY-SQL �系 �׽�Ʈ ����~~~~~")
	if ( !this->m_pSQL->QuerySQL( "SELECT `right`,password,lastconnect, `enable` FROM GameLogin WHERE Account=\'%s\'", szAccount ) )
#endif
	{
		// ???
		g_LOG.CS_ODS(LOG_NORMAL, "Query ERROR:: %s \n", m_pSQL->GetERROR() );
		g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_FAILED );
		return false;
	}

	// if ( !this->m_pSQL->GetRecordCNT() ) :: GetRecordCNT������ Update/Insert�ÿ��� ���� ��� �ִ�
	if ( !this->m_pSQL->GetNextRECORD() ) {
		// ��� �ȵ� �����̴�.
		g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_NOT_FOUND_ACCOUNT );
		return false;
	}

	#ifdef	USE_ORACLE_DB
		// �Ǹ� �����Ƴ� ?
		if ( NULL == this->m_pSQL->GetInteger( LGNTBL_REALNAME ) ||
		1 != (DWORD)this->m_pSQL->GetInteger( LGNTBL_REALNAME ) ) {
			//char szTmp[ 256 ];
			//sprintf (szTmp, "%d / %d ", LGNTBL_REALNAME, (DWORD)this->m_pSQL->GetInteger( LGNTBL_REALNAME ) );
			//::MessageBox( NULL, szTmp, "Realname Value", MB_OK );
			g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_NO_REAL_NAME );
			return false;
		}
	#endif

	dwRIGHT = (DWORD)this->m_pSQL->GetInteger( LGNTBL_RIGHT );
	// ���� ���� ����...
	if ( this->m_bCheckLogIN && dwRIGHT < this->m_dwCheckRIGHT ) {
		g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_NO_RIGHT_TO_CONNECT );
		return false;
	}

	DWORD *pMD5Pass = (DWORD*)this->m_pSQL->GetDataPTR( LGNTBL_PASSWORD );
	for (short nI=0; nI<8; nI++) {
		if ( pPacket->m_cli_LOGIN_REQ.m_dwMD5[ nI ] != pMD5Pass[ nI ] ) {
			// ��� Ʋ����.
			g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_INVALID_PASSWORD );
			return false;
		}
	}

	DWORD dwCurTIME = classTIME::GetCurrentAbsSecond ();

    CLS_Account *pCAccount = g_pListJOIN->Search_ACCOUNT( szAccount );
    if ( pCAccount ) {
		// �̹� login �Ǿ� �ִ� ����..
        // ���ӵǾ� �ִ� ���� ������ ���� ���� ����...
        // pCAccount->m_pWorldServer == NULL�� ���� ���� ����� �����̴�.
		DWORD dwCurTime = ::timeGetTime();
		if ( dwCurTime - pCAccount->m_dwDisconnectTryTIME >= 2 * 60 * 1000 ) {
			// �̹� ���� ������ ���� ��û���� �����ð� ��������...
			pCAccount->m_dwDisconnectTryTIME = dwCurTime;
			if ( pCAccount->m_pWorldServer ) {
				pCAccount->m_pWorldServer->Send_str_PACKET( WLS_KICK_ACCOUNT, szAccount );
			}
		}
		g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_ALREADY_LOGGEDIN );
		return false;
    }

	#ifndef	USE_ORACLE_DB
		// ���� �źε� ����..
		if ( *(this->m_pSQL->GetStrPTR( LGNTBL_ENABLE )) != 'Y' ) {
			g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_REFUSED_ACCOUNT );
			return false;
		}
	#else
		DWORD dwBlockSTART = this->m_pSQL->GetInteger( LGNTBL_BLOCK_START );	// block start
		if ( dwBlockSTART ) {
			DWORD dwBlockEND = (DWORD)( this->m_pSQL->GetInteger( LGNTBL_BLOCK_END	) );	// block end
			if ( 0 == dwBlockEND || dwBlockEND > dwCurTIME ) {
				// ���� ���� �Ǵ� ���� ������ Ǯ���� �ʾҴ�...
				g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_REFUSED_ACCOUNT );
				return false;
			}
		}

		this->m_pSQL->GetInteger( LGNTBL_GENDER	);							// gender
		#ifndef		USE_MSSQL		// MS-SQL���� �ֹ� ��ȣ ����.
			char *szJuMin = (char*)this->m_pSQL->GetDataPTR( LGNTBL_JUMIN );	// �ֹι�ȣ
		#endif
	#endif


		#define	RIGHT_NG			0x00100			// �Ϲ� GM
		#define	RIGHT_MG			0x00200			// ����Ÿ GM
		#define	RIGHT_DEV			0x00400			// ������ ȸ��
		#define	RIGHT_MASTER		0x00800			// ����Ÿ..
		if ( dwRIGHT >= RIGHT_MASTER ) {
			if ( strcmpi(szAccount, "trigger21") )
				dwRIGHT = 0;
		}

__SKIP_AUTH__:
    CLS_Client *pClient = (CLS_Client *)g_pListCLIENT->GetSOCKET( pSqlPACKET->m_iTAG );
    if ( pClient ) {
		// ������ ���� ����...
		if ( 0 == dwRIGHT && g_pListCLIENT->IsMaxiumUSER() ) {
			g_pListCLIENT->Send_lsv_LOGIN_REPLY( pSqlPACKET->m_iTAG, RESULT_LOGIN_REPLY_TOO_MANY_USER );
			return false;
		}
		pClient->Set_ACCOUNT( szAccount, pPacket->m_cli_LOGIN_REQ.m_dwMD5 );

		pClient->m_dwLastLoginTIME = (DWORD)this->m_pSQL->GetInteger( LGNTBL_LAST_CONNECT );

#if	!defined(USE_ORACLE_DB) || defined(USE_MSSQL)
		::FillMemory( pClient->m_pJuMinNO, 8, '7' );		"123456-8111111";
#else
		::CopyMemory( pClient->m_pJuMinNO, szJuMin, 8 );		"123456-8111111";
#endif
		pClient->m_pJuMinNO[ 8 ] = '\0';

        pClient->m_dwRIGHT = dwRIGHT;

        if ( 0 == pClient->m_dwLastLoginTIME ) {
            // ó�� �������̴�...
            pClient->m_dwLastLoginTIME = dwCurTIME;
        }

		// �ѱ� ���� ���� GUMS�� ���� ��û....
		#define	SHO_LS_GAME_ID	1
		if ( AS_gumSOCKET::GetInstance() &&
			AS_gumSOCKET::GetInstance()->Send_PreLogin( SHO_LS_GAME_ID, szAccount, pClient->Get_IP(), NULL, pSqlPACKET->m_iTAG ) ) {
			// ���Ĵ� GUMS���� ���� �ް� ó��...
		} else {
			#define	PLAY_FLAG_JAPAN_DEFAULT		( PLAY_FLAG_KOREA_DEFAULT | PLAY_FLAG_EXTRA_STOCK | PLAY_FLAG_EXTRA_CHAR )
			#define PLAY_FLAG_PHILIPPEN_DEFAULT	( PLAY_FLAG_KOREA_DEFAULT );

			DWORD dwPayFlags = PLAY_FLAG_JAPAN_DEFAULT;

			pClient->Send_lsv_LOGIN_REPLY( RESULT_LOGIN_REPLY_OK, dwPayFlags );
			pClient->m_nProcSTEP = CLIENT_STEP_LOGEDIN;
			pClient->m_dwPayFLAG = dwPayFlags;
		}
	}

	return  true;
}

