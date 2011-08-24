

//+----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2008
//
//  Description:
//      ISilverlightViewer and IRenderTargetBitmap interface definitions
//
//-----------------------------------------------------------------------------
struct XRECT
{
    int X;
    int Y;
    int Width;
    int Height;
};

//------------------------------------------------------------------------
//
//  Interface:  IRenderTargetBitmap
//
//  Synopsis:
//      Interface for drawing a subtree to a surface pointer
//
//------------------------------------------------------------------------
struct IRenderTargetBitmap
{
    //
    // Ref counting
    //

    virtual  unsigned int __stdcall AddRef() = 0;
    virtual  unsigned int __stdcall Release() = 0;

    //
    // Render call - returns the dirty rectangle
    //

    virtual  HRESULT __stdcall Render(
        __in void *pElement,
        __out XRECT *prcDirtyRect
        ) = 0;
};


//------------------------------------------------------------------------
//
//  Interface:  ISilverlightViewer
//
//  Synopsis:
//      The interface that allows drawing the Silverlight content 
//      and feeding input to the Silverlight control.
//
//------------------------------------------------------------------------
struct ISilverlightViewer 
{
    //
    // Ref counting
    //

    virtual unsigned int __stdcall AddRef() = 0;
    virtual unsigned int __stdcall Release () = 0;

    virtual void    __stdcall SuppressBrowserEvents(__in int bSuppress) = 0;

    //
    // Create a IRenderTargetBitmap
    //

    virtual  unsigned int __stdcall CreateRenderTargetBitmap(
        __in unsigned int uBackground,
        __in int nOffsetX,
        __in int nOffsetY,
        __in unsigned int nWidth,
        __in unsigned int nHeight,
        __in void *pvBits,
        __out IRenderTargetBitmap **ppIRenderTargetBitmap
        )= 0;


    virtual  HRESULT __stdcall ProcessInput(
        __in unsigned int msg,
        __in unsigned int wParam,
        __in unsigned int lParam,
        __in void* curEvent,
        __in int x,
        __in int y,
        __out unsigned int *fHandled
    )= 0;



};

//------------------------------------------------------------------------
//
//  Interface:  ISilverlightViewerProvider
//
//  Synopsis:
//      Gets the ISilverlightViewer interface
//
//------------------------------------------------------------------------
struct __declspec(uuid("ed114498-ed0c-46c9-8609-e480a7e8b085"))
ISilverlightViewerProvider : public IUnknown
{
    //
    // Get the ISilverlightViewer interface
    //

    STDMETHOD(GetSilverlightViewer)(
        __out ISilverlightViewer **ppSilverlightViewer
        ) = 0;
};
