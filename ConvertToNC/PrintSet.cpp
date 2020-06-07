#include "StdAfx.h"
#include "dbplotsetval.h"
#include "CadToolFunc.h"
#include "BatchPrint.h"

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
            if (sTemp.CompareNoCase(sDeviceName))
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
{	//判断打印机是否合理
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
        if (retCode != Acad::eOk || stricmp(sPlotCfgName, "无") == 0)
        {	//获取输出设备名称
            if (bPromptInfo)
            {
#ifdef _ARX_2007
                acutPrintf(L"\n当前激活打印设备不可用,请优先进行页面设置!");
#else
                acutPrintf("\n当前激活打印设备不可用,请优先进行页面设置!");
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
        //ACHAR* m_strDevice = _T("DWF6 ePlot.pc3");//打印机名字
#ifdef _ARX_2007
        Acad::ErrorStatus es=pPsv->setPlotCfgName(pLayout, _bstr_t(pPlotCfg->m_sDeviceName));     //设置打印设备
#else
        Acad::ErrorStatus es = pPsv->setPlotCfgName(pLayout, pPlotCfg->m_sDeviceName);            //设置打印设备
#endif
        if (es != Acad::eOk)
        {	//获取输出设备名称
            if (bPromptInfo)
            {
                CXhChar500 sError("\n名称:%s,设置错误，请在设置中确认后重试！", (char*)pPlotCfg->m_sDeviceName);
#ifdef _ARX_2007
                acutPrintf(_bstr_t(sError));
#else
                acutPrintf(sError);
#endif
            }
        }
        //ACHAR* m_mediaName = _T("ISO A4");//图纸名称
#ifdef _ARX_2007
        pPsv->setCanonicalMediaName(pLayout, _bstr_t(pPlotCfg->m_sPaperSize));//设置图纸尺寸
		pPsv->setCurrentStyleSheet(pLayout, L"monochrome.ctb");//设置打印样式表
#else
		pPsv->setCanonicalMediaName(pLayout, pPlotCfg->m_sPaperSize);//设置图纸尺寸
		pPsv->setCurrentStyleSheet(pLayout, _T("monochrome.ctb"));//设置打印样式表
#endif
        pPsv->setPlotType(pLayout, AcDbPlotSettings::kWindow);//设置打印范围为窗口
        pPsv->setPlotWindowArea(pLayout, 100, 100, 200, 200);//设置打印范围,超出给范围的将打不出来
        pPsv->setPlotCentered(pLayout, true);//是否居中打印
        pPsv->setUseStandardScale(pLayout, true);//设置是否采用标准比例
        pPsv->setStdScaleType(pLayout, AcDbPlotSettings::kScaleToFit);//布满图纸
        pPsv->setPlotRotation(pLayout, AcDbPlotSettings::k0degrees);//设置打印方向
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