/////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/joystick.h
// Purpose:     wxJoystick class
// Author:      Guilhem Lavaux
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id$
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_JOYSTICK_H_
#define _WX_UNIX_JOYSTICK_H_

#include "wx/event.h"

class  wxJoystickThread;

class  wxJoystick: public wxObject
{
    DECLARE_DYNAMIC_CLASS(wxJoystick)
        public:
    /*
     * Public interface
     */

    wxJoystick(int joystick = wxJOYSTICK1);
    virtual ~wxJoystick();

    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    wxPoint GetPosition() const;
    int GetPosition(unsigned axis) const;
    bool GetButtonState(unsigned button) const;
    int GetZPosition() const;
    int GetButtonState() const;
    int GetPOVPosition() const;
    int GetPOVCTSPosition() const;
    int GetRudderPosition() const;
    int GetUPosition() const;
    int GetVPosition() const;
    int GetMovementThreshold() const;
    void SetMovementThreshold(int threshold) ;

    // Capabilities
    ////////////////////////////////////////////////////////////////////////////

    bool IsOk() const; // Checks that the joystick is functioning
    static int GetNumberJoysticks() ;
    int GetManufacturerId() const ;
    int GetProductId() const ;
    wxString GetProductName() const ;
    int GetXMin() const;
    int GetYMin() const;
    int GetZMin() const;
    int GetXMax() const;
    int GetYMax() const;
    int GetZMax() const;
    int GetNumberButtons() const;
    int GetNumberAxes() const;
    int GetMaxButtons() const;
    int GetMaxAxes() const;
    int GetPollingMin() const;
    int GetPollingMax() const;
    int GetRudderMin() const;
    int GetRudderMax() const;
    int GetUMin() const;
    int GetUMax() const;
    int GetVMin() const;
    int GetVMax() const;

    bool HasRudder() const;
    bool HasZ() const;
    bool HasU() const;
    bool HasV() const;
    bool HasPOV() const;
    bool HasPOV4Dir() const;
    bool HasPOVCTS() const;

    // Operations
    ////////////////////////////////////////////////////////////////////////////

    // pollingFreq = 0 means that movement events are sent when above the threshold.
    // If pollingFreq > 0, events are received every this many milliseconds.
    bool SetCapture(wxWindow* win, int pollingFreq = 0);
    bool ReleaseCapture();

protected:
    int                 m_device;
    int                 m_joystick;
    wxJoystickThread*   m_thread;
};

#endif // _WX_UNIX_JOYSTICK_H_
