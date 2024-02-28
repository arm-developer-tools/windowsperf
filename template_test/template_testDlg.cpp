
// template_testDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "template_test.h"
#include "template_testDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


enum status_flag            // Status of the driver lock
{
    STS_IDLE,               // When no process has the driver locked aka "the default state".
    STS_BUSY,               // When another process already has the driver lock.
    STS_LOCK_AQUIRED,       // When the calling process has successfully acquired the driver lock.
};

enum lock_flag
{
    LOCK_GET,           // Ask to lock the driver and acquire "session"
    LOCK_GET_FORCE,     // Force driver to release the driver and lock it with the current process
    LOCK_RELEASE,       // Release the driver after acquired lock
};

struct lock_request
{
    enum pmu_ctl_action action;
    enum lock_flag flag;
};


// From handy wdm.h macros
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED 0
#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define PMU_CTL_ACTION_OFFSET 0x900

#define WPERF_TYPE 40004
enum pmu_ctl_action
{
    PMU_CTL_START = PMU_CTL_ACTION_OFFSET,
    PMU_CTL_STOP,
    PMU_CTL_RESET,
    PMU_CTL_QUERY_HW_CFG,
    PMU_CTL_QUERY_SUPP_EVENTS,
    PMU_CTL_QUERY_VERSION,
    PMU_CTL_ASSIGN_EVENTS,
    PMU_CTL_READ_COUNTING,
    DSU_CTL_INIT,
    DSU_CTL_READ_COUNTING,
    DMC_CTL_INIT,
    DMC_CTL_READ_COUNTING,
    PMU_CTL_SAMPLE_SET_SRC,
    PMU_CTL_SAMPLE_START,
    PMU_CTL_SAMPLE_STOP,
    PMU_CTL_SAMPLE_GET,
    PMU_CTL_LOCK_ACQUIRE,
    PMU_CTL_LOCK_RELEASE,
};


#define IOCTL_PMU_CTL_START                          CTL_CODE(WPERF_TYPE,  PMU_CTL_START , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_STOP                            CTL_CODE(WPERF_TYPE,  PMU_CTL_STOP , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_RESET                          CTL_CODE(WPERF_TYPE,  PMU_CTL_RESET , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_HW_CFG           CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_HW_CFG , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_SUPP_EVENTS   CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_SUPP_EVENTS , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_QUERY_VERSION 	       CTL_CODE(WPERF_TYPE,  PMU_CTL_QUERY_VERSION , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_ASSIGN_EVENTS 	       CTL_CODE(WPERF_TYPE,  PMU_CTL_ASSIGN_EVENTS , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_READ_COUNTING 	       CTL_CODE(WPERF_TYPE,  PMU_CTL_READ_COUNTING , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DSU_CTL_INIT 	                           CTL_CODE(WPERF_TYPE,  DSU_CTL_INIT , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DSU_CTL_READ_COUNTING 	       CTL_CODE(WPERF_TYPE,  DSU_CTL_READ_COUNTING , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DMC_CTL_INIT 	                           CTL_CODE(WPERF_TYPE,  DMC_CTL_INIT , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_DMC_CTL_READ_COUNTING	       CTL_CODE(WPERF_TYPE,  DMC_CTL_READ_COUNTING , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_SET_SRC 	       CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_SET_SRC , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_START 	           CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_START , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_STOP 	           CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_STOP , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_SAMPLE_GET 	               CTL_CODE(WPERF_TYPE,  PMU_CTL_SAMPLE_GET , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_LOCK_ACQUIRE	           CTL_CODE(WPERF_TYPE,  PMU_CTL_LOCK_ACQUIRE , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_PMU_CTL_LOCK_RELEASE 	           CTL_CODE(WPERF_TYPE,  PMU_CTL_LOCK_RELEASE , METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CtemplatetestDlg dialog



CtemplatetestDlg::CtemplatetestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TEMPLATE_TEST_DIALOG, pParent)
    , m_devsVal(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtemplatetestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_CBString(pDX, IDC_COMBO1, m_devsVal);
    DDX_Control(pDX, IDC_COMBO1, m_devsCtl);
}

BEGIN_MESSAGE_MAP(CtemplatetestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_CBN_SELCHANGE(IDC_COMBO1, &CtemplatetestDlg::OnCbnSelchangeCombo1)
    ON_BN_CLICKED(IDC_TESTLOCK, &CtemplatetestDlg::OnBnClickedTestlock)
    ON_BN_CLICKED(IDC_TESTLOCKFORCE, &CtemplatetestDlg::OnBnClickedTestlockforce)
    ON_BN_CLICKED(IDC_TESTUNLOCK, &CtemplatetestDlg::OnBnClickedTestunlock)
    ON_BN_CLICKED(IDC_CLOSE, &CtemplatetestDlg::OnBnClickedClose)
    ON_BN_CLICKED(IDC_OPEN, &CtemplatetestDlg::OnBnClickedOpen)
END_MESSAGE_MAP()


// CtemplatetestDlg message handlers

BOOL CtemplatetestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    m_h = NULL;

	// TODO: Add extra initialization here
    GetDevices();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CtemplatetestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CtemplatetestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CtemplatetestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL
CtemplatetestDlg::GetDevices(void)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    LPGUID InterfaceGuid = (LPGUID)&GUID_DEVINTERFACE_WINDOWSPERF;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) 
    {
        MessageBox(L"Error:", L"0x%x retrieving device interface list size", MB_OK| MB_ICONEXCLAMATION);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) 
    {
        bRet = FALSE;
        MessageBox(L"Error: No active device interfaces found",
            L"Is the driver loaded? You can get the latest driver from https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/releases", MB_OK | MB_ICONEXCLAMATION);
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) 
    {
        MessageBox(L"Error:", L"Allocating memory for device interface list", MB_OK | MB_ICONEXCLAMATION);
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
    {
        MessageBox(L"Error:",  L" 0x%x retrieving device interface list", MB_OK | MB_ICONEXCLAMATION);
        goto clean0;
    }

    PWSTR deviceInterface;

    for (deviceInterface = deviceInterfaceList; *deviceInterface; deviceInterface += wcslen(deviceInterface) + 1)
    {
        DEVPROPTYPE devicePropertyType;
        ULONG deviceHWIdListLength = 256;
        WCHAR deviceHWIdList[256];
        CONFIGRET cr1 = CR_SUCCESS;

        ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
        WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

        cr1 = CM_Get_Device_Interface_Property(
            deviceInterface,
            &DEVPKEY_Device_InstanceId,
            &devicePropertyType,
            (PBYTE)deviceInstanceId,
            &deviceInstanceIdLength,
            0);
        if (cr1 != CR_SUCCESS)
        {
            UINT err = GetLastError();
            continue;
        }

        // add the string to the dorp box
        m_devsCtl.AddString(deviceInterface);
        m_devsCtl.SetCurSel(0);

        DEVINST deviceInstanceHandle;

        cr1 = CM_Locate_DevNode(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL);
        if (cr1 != CR_SUCCESS)
        {
            UINT err = GetLastError();
            continue;
        }

        cr1 = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_HardwareIds,
            &devicePropertyType,
            (PBYTE)deviceHWIdList,
            &deviceHWIdListLength,
            0);
        if (cr1 != CR_SUCCESS)
        {
            UINT err = GetLastError();
            continue;
        }

        // hardware IDs is a null sperated list of strings per device, pull it apart
        PWSTR hwId;
        for (hwId = deviceHWIdList; *hwId; hwId += wcslen(hwId) + 1)
        {
            // do something with hwId
        }
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

   

    return bRet;
}

void CtemplatetestDlg::OnCbnSelchangeCombo1()
{
    m_devsCtl.GetLBText(m_devsCtl.GetCurSel(), m_strSel);
    UpdateData(FALSE);
}


void CtemplatetestDlg::OnBnClickedClose()
{
    if (m_h &&  (m_h !=INVALID_HANDLE_VALUE))
        CloseHandle(m_h);
}


void CtemplatetestDlg::OnBnClickedOpen()
{
    int i = m_devsCtl.GetCurSel();

    if (m_h && (m_h != INVALID_HANDLE_VALUE))
        CloseHandle(m_h);

    m_h = CreateFile(m_strSel,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
}



void CtemplatetestDlg::OnBnClickedTestlock()
{
    if (m_h && (m_h != INVALID_HANDLE_VALUE))
    {
        struct lock_request req;
        req.flag = LOCK_GET;
        DWORD resplen = 0;
        enum status_flag sts_flag = STS_BUSY;
        BOOL status = DeviceIoControl(m_h, IOCTL_PMU_CTL_LOCK_ACQUIRE, &req, sizeof(lock_request), &sts_flag, sizeof(enum status_flag), &resplen, NULL);
        if (!status)
            MessageBox(L"Error", L"DeviceIoControl  failed", MB_OK);

        else
        {
            if (sts_flag != STS_LOCK_AQUIRED)
                MessageBox(L"Error", L"lock not aquired", MB_OK);
            else
                MessageBox(L"Success", L"Success", MB_OK);
        }
    }
}


void CtemplatetestDlg::OnBnClickedTestlockforce()
{
    if (m_h && (m_h != INVALID_HANDLE_VALUE))
    {
        struct lock_request req;
        req.flag = LOCK_GET_FORCE;
        DWORD resplen = 0;
        enum status_flag sts_flag = STS_BUSY;
        BOOL status = DeviceIoControl(m_h, IOCTL_PMU_CTL_LOCK_ACQUIRE, &req, sizeof(lock_request), &sts_flag, sizeof(enum status_flag), &resplen, NULL);
        if (!status)
            MessageBox(L"Error", L"DeviceIoControl  failed", MB_OK);

        else
        {
            if (sts_flag != STS_LOCK_AQUIRED)
                MessageBox(L"Error", L"lock not aquired", MB_OK);
            else
                MessageBox(L"Success", L"Success", MB_OK);
        }
    }
}


void CtemplatetestDlg::OnBnClickedTestunlock()
{
    if (m_h && (m_h != INVALID_HANDLE_VALUE))
    {
        struct lock_request req;
        DWORD resplen = 0;
        enum status_flag sts_flag = STS_BUSY;
        req.flag = LOCK_RELEASE;

        BOOL status = DeviceIoControl(m_h, IOCTL_PMU_CTL_LOCK_RELEASE, &req, sizeof(lock_request), &sts_flag, sizeof(enum status_flag), &resplen, NULL);
        if (!status)
            MessageBox(L"Error", L"DeviceIoControl  failed", MB_OK);

        else
        {
            if (sts_flag != STS_IDLE)
                MessageBox(L"Error", L"lock not released", MB_OK);
            else
                MessageBox(L"Success", L"Success", MB_OK);
        }
    }
}



