#ifndef _CAVATARINFODLG_
#define _CAVATARINFODLG_

#include "tgamectrl/tdialog.h"
#include "../gamecommon/IObserver.h"
#include "subclass/CSlot.h"

class CItem;

/**
* È­¸é ¿ŞÂÊÀ§ ³» ¾ÆÅ¸¹Ù¿¡ ´ëÇÑ Á¤º¸¸¦ Ç¥½ÃÇÏ´Â ÀÎÅÍÆäÀÌ½º
*	- Observable : CItemSlot
*
* @Author		ÃÖÁ¾Áø
* @Date			2005/9/6
*/
class CAvatarInfoDlg : public CTDialog, public IObserver
{
public:
	CAvatarInfoDlg( int iType );
	virtual ~CAvatarInfoDlg(void);
	virtual bool Create ( const char* IDD );
	virtual void Draw();
	virtual void Update( POINT ptMouse );
	virtual void MoveWindow( POINT pt );
	virtual unsigned Process(UINT uiMsg,WPARAM wParam,LPARAM lParam);
	virtual void Update( CObservable* pObservable, CTObject* pObj );
	
	virtual void Show();
	virtual void Hide();
	virtual void RefreshDlg();
	virtual void SetInterfacePos_After();

	void		SetMaxView(bool bIsMax = true);
	void		SetMinView();
	bool		GetIsMaxView();

	void		SetMiniMapPos();
	

	enum
	{
		IID_GUAGE_HP = 6,					/// Ã¼·Â°ÔÀÌÁö
		IID_GUAGE_MP,						/// ¸¶³ª°ÔÀÌÁö
		IID_GUAGE_EXP,						/// °æÇèÄ¡°ÔÀÌÁö
		IID_BTN_SELFTARGET = 10,			/// Å¬¸¯½Ã ÀÚ½Å¼±ÅÃ - NOIMAGE 
		IID_BTN_MENU,						/// ¸Ş´ºÃ¢À» ¿©´Â ¹öÆ°
		IID_BTN_DIALOG2ICON,				/// ¸ğµç Dialog IconÈ­ ¹öÆ°, IconÈ­ ºÒ°¡´ÉÇÑ Dialog´Â ´ç±Ù µ¿ÀÛ ¾ÈÇÑ´Ù.(TGameCtrl¿¡¼­ Ã³¸®ÇÒ°Í, ¸â¹ö·Î iconÈ­ °¡´ÉÇÑÁö SettingÇÒ°Í )
		IID_BTN_SCREENSHOT,					/// ½ºÅ©¸°¼¦ Âï´Â ¹öÆ°
		IID_BTN_EXP							/// Adicionado botão para selecionar a taxa de EXP.
	};
	

private:
	CSlot	m_Slot;							/// ÀåÂøµÈ ¹«±â¿¡ ´ëÇÑ ¾ÆÀÌÄÜÀÌ µé¾î°¥ ½½·Ô
	CItem*	m_pRepresentBullet;				/// ÀåÂøµÈ ¹«±â¿¡ ´ëÇÑ ¼Ò¸ğÅº
	bool	m_bIsMaxView;					/// È®´ë or Ãà¼Ò

};
#endif