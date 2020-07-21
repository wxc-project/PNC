#include "StdAfx.h"
#include "dbplotsetval.h"
#include "CadToolFunc.h"
#include "BatchPrint.h"

#if defined(__UBOM_) || defined(__UBOM_ONLY_)
CString CPrintSet::GetPrintDeviceCmbItemStr()
{
    CString sDeviceStr;
    CLockDocumentLife lock;
    AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
    //get the active layout
    AcDbLayout* pLayout = pLayMan?pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE):NULL;
    AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
    if (pLayout && pPsv)
    {
        pPsv->refreshLists(pLayout);        //refresh the Plot Config list
        AcArray<const ACHAR*> mDeviceList;
        pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
        sDeviceStr.Empty();
        for (int i = 0; i < mDeviceList.length(); i++)
        {
            CString sTemp = mDeviceList.at(i);
            if (sDeviceStr.GetLength() > 0)
                sDeviceStr.AppendChar('|');
            sDeviceStr.Append(sTemp);
        }
    }
    if (pLayout)
        pLayout->close();
    return sDeviceStr;
}

CString CPrintSet::GetMediaNameByDeviceName(const char* sDeviceName)
{
    CString sMediaNameStr;
    CLockDocumentLife lock;
    AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
    //get the active layout
    AcDbLayout* pLayout = pLayMan ? pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE) : NULL;
    AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
    if (pLayout && pPsv && sDeviceName)
    {
        pPsv->refreshLists(pLayout);        //refresh the Plot Config list
        AcArray<const ACHAR*> mDeviceList;
        pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
        BOOL bValidName = FALSE;
        for (int i = 0; i < mDeviceList.length(); i++)
        {
            CString sTemp = mDeviceList.at(i);
			if (sTemp.CompareNoCase(sDeviceName) == 0)
			{
				bValidName = TRUE;
				break;
			}
        }
        if (bValidName)
        {
#ifdef _ARX_2007
            pPsv->setPlotCfgName(pLayout, _bstr_t(sDeviceName));
#else
			pPsv->setPlotCfgName(pLayout, sDeviceName);
#endif
            //list all the paper sizes in the given Plot configuration
            AcArray<const ACHAR*> mMediaList;
            pPsv->canonicalMediaNameList(pLayout, mMediaList);
            for (int i = 0; i < mMediaList.length(); i++)
            {
                CString sTemp = mMediaList.at(i);
                if (sMediaNameStr.GetLength() > 0)
                    sMediaNameStr.AppendChar('|');
                sMediaNameStr.Append(sTemp);
            }
        }
    }
    if(pLayout)
        pLayout->close();
    return sMediaNameStr;
}


CXhChar500 CPrintSet::GetPlotCfgName(bool bPromptInfo)
{	//�жϴ�ӡ���Ƿ����
    CXhChar500 sPlotCfgName;
    AcApLayoutManager* pLayMan = NULL;
    pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
    AcDbLayout* pLayout = pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE);
    if (pLayout != NULL)
    {
        pLayout->close();
        AcDbPlotSettings* pPlotSetting = (AcDbPlotSettings*)pLayout;
        Acad::ErrorStatus retCode;
#ifdef _ARX_2007
        const ACHAR* sValue;
        retCode = pPlotSetting->getPlotCfgName(sValue);
        if (sValue != NULL)
            sPlotCfgName.Copy((char*)_bstr_t(sValue));
#else
        char* sValue;
        retCode = pPlotSetting->getPlotCfgName(sValue);
        if (sValue != NULL)
            sPlotCfgName.Copy(sValue);
#endif
        if (retCode != Acad::eOk || stricmp(sPlotCfgName, "��") == 0)
        {	//��ȡ����豸����
            if (bPromptInfo)
            {
#ifdef _ARX_2007
                acutPrintf(L"\n��ǰ�����ӡ�豸������,�����Ƚ���ҳ������!");
#else
                acutPrintf("\n��ǰ�����ӡ�豸������,�����Ƚ���ҳ������!");
#endif
            }
            sPlotCfgName.Empty();
        }
    }
    return sPlotCfgName;
}

BOOL CPrintSet::SetPlotMedia(PLOT_CFG *pPlotCfg, bool bPromptInfo)
{
    CLockDocumentLife lock;
    AcApLayoutManager* pLayMan = (AcApLayoutManager*)acdbHostApplicationServices()->layoutManager();
    //get the active layout
    AcDbLayout* pLayout = pLayMan ? pLayMan->findLayoutNamed(pLayMan->findActiveLayout(TRUE), TRUE) : NULL;
    AcDbPlotSettingsValidator* pPsv = acdbHostApplicationServices()->plotSettingsValidator();
    if (pLayout && pPsv)
    {
        pPsv->refreshLists(pLayout);        //refresh the Plot Config list
        //
        //AcArray<const ACHAR*> mDeviceList;
        //pPsv->plotDeviceList(mDeviceList);  //get all the Plot Configurations
        //ACHAR* m_strDevice = _T("DWF6 ePlot.pc3");//��ӡ������
#ifdef _ARX_2007
        Acad::ErrorStatus es=pPsv->setPlotCfgName(pLayout, _bstr_t(pPlotCfg->m_sDeviceName));     //���ô�ӡ�豸
#else
        Acad::ErrorStatus es = pPsv->setPlotCfgName(pLayout, pPlotCfg->m_sDeviceName);            //���ô�ӡ�豸
#endif
        if (es != Acad::eOk)
        {	//��ȡ����豸����
            if (bPromptInfo)
            {
                CXhChar500 sError("\n����:%s,���ô�������������ȷ�Ϻ����ԣ�", (char*)pPlotCfg->m_sDeviceName);
#ifdef _ARX_2007
                acutPrintf(_bstr_t(sError));
#else
                acutPrintf(sError);
#endif
            }
        }
        //ACHAR* m_mediaName = _T("ISO A4");//ͼֽ����
#ifdef _ARX_2007
        pPsv->setCanonicalMediaName(pLayout, _bstr_t(pPlotCfg->m_sPaperSize));//����ͼֽ�ߴ�
		pPsv->setCurrentStyleSheet(pLayout, L"monochrome.ctb");//���ô�ӡ��ʽ��
#else
		pPsv->setCanonicalMediaName(pLayout, pPlotCfg->m_sPaperSize);//����ͼֽ�ߴ�
		pPsv->setCurrentStyleSheet(pLayout, _T("monochrome.ctb"));//���ô�ӡ��ʽ��
#endif
        pPsv->setPlotType(pLayout, AcDbPlotSettings::kWindow);//���ô�ӡ��ΧΪ����
        pPsv->setPlotWindowArea(pLayout, 100, 100, 200, 200);//���ô�ӡ��Χ,��������Χ�Ľ��򲻳���
        pPsv->setPlotCentered(pLayout, true);//�Ƿ���д�ӡ
        pPsv->setUseStandardScale(pLayout, true);//�����Ƿ���ñ�׼����
        pPsv->setStdScaleType(pLayout, AcDbPlotSettings::kScaleToFit);//����ͼֽ
        pPsv->setPlotRotation(pLayout, AcDbPlotSettings::k0degrees);//���ô�ӡ����
        //
        pLayout->close();
        return (es == Acad::eOk);
    }
    else
        return FALSE;
    /*
    int nLength = mDeviceList.length();
    //get the user input for listing the Media Names
    //select the selected Plot configuration
    pPsv->setPlotCfgName(pLayout, mDeviceList.at(--nSel));
    //list all the paper sizes in the given Plot configuration
    AcArray<const ACHAR*> mMediaList;
    const ACHAR* pLocaleName;
    pPsv->canonicalMediaNameList(pLayout, mMediaList);

    nLength = mMediaList.length();
    int nCtr = 0;
    for (nCtr = 0; nCtr < nLength; nCtr++)
    {   //get the localename
        pPsv->getLocaleMediaName(pLayout, mMediaList.at(nCtr), pLocaleName);
    }
    */
}
#endif