#include "binding_capture_button.hpp"

#include <QFocusEvent>
#include <QKeyEvent>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "arrower/key_codes.hpp"

namespace arrower {

namespace {

int FallbackVirtualKey(int qt_key) {
    if (qt_key >= Qt::Key_A && qt_key <= Qt::Key_Z) {
        return 'A' + (qt_key - Qt::Key_A);
    }

    if (qt_key >= Qt::Key_0 && qt_key <= Qt::Key_9) {
        return '0' + (qt_key - Qt::Key_0);
    }

    switch (qt_key) {
        case Qt::Key_Left:
            return VK_LEFT;
        case Qt::Key_Up:
            return VK_UP;
        case Qt::Key_Right:
            return VK_RIGHT;
        case Qt::Key_Down:
            return VK_DOWN;
        case Qt::Key_Control:
            return VK_CONTROL;
        case Qt::Key_Alt:
            return VK_MENU;
        case Qt::Key_Shift:
            return VK_SHIFT;
        case Qt::Key_Escape:
            return VK_ESCAPE;
        case Qt::Key_Tab:
            return VK_TAB;
        case Qt::Key_Space:
            return VK_SPACE;
        case Qt::Key_PageUp:
            return VK_PRIOR;
        case Qt::Key_PageDown:
            return VK_NEXT;
        case Qt::Key_Home:
            return VK_HOME;
        case Qt::Key_End:
            return VK_END;
        case Qt::Key_Insert:
            return VK_INSERT;
        case Qt::Key_Delete:
            return VK_DELETE;
        case Qt::Key_Period:
            return VK_OEM_PERIOD;
        case Qt::Key_Slash:
            return VK_OEM_2;
        case Qt::Key_Comma:
            return VK_OEM_COMMA;
        case Qt::Key_Semicolon:
            return VK_OEM_1;
        case Qt::Key_Minus:
            return VK_OEM_MINUS;
        case Qt::Key_Equal:
            return VK_OEM_PLUS;
        default:
            return 0;
    }
}

int EventVirtualKey(QKeyEvent* event) {
    const quint32 native_key = event->nativeVirtualKey();
    if (native_key > 0 && native_key <= 0xFF) {
        return static_cast<int>(native_key);
    }

    return FallbackVirtualKey(event->key());
}

}  // namespace

BindingCaptureButton::BindingCaptureButton(QWidget* parent) : QPushButton(parent) {
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &QPushButton::clicked, this, &BindingCaptureButton::StartCapture);
    UpdateText();
}

void BindingCaptureButton::SetVirtualKey(int virtual_key) {
    virtual_key_ = virtual_key;
    UpdateText();
}

int BindingCaptureButton::VirtualKey() const {
    return virtual_key_;
}

void BindingCaptureButton::keyPressEvent(QKeyEvent* event) {
    if (!capturing_) {
        QPushButton::keyPressEvent(event);
        return;
    }

    const int virtual_key = EventVirtualKey(event);
    if (virtual_key == 0) {
        return;
    }

    capturing_ = false;
    releaseKeyboard();
    if (virtual_key_ != virtual_key) {
        virtual_key_ = virtual_key;
        emit VirtualKeyChanged(virtual_key_);
    }
    UpdateText();
}

void BindingCaptureButton::focusOutEvent(QFocusEvent* event) {
    if (capturing_) {
        StopCapture();
    }
    QPushButton::focusOutEvent(event);
}

void BindingCaptureButton::StartCapture() {
    capturing_ = true;
    setText("Press key...");
    grabKeyboard();
    setFocus(Qt::MouseFocusReason);
}

void BindingCaptureButton::StopCapture() {
    capturing_ = false;
    releaseKeyboard();
    UpdateText();
}

void BindingCaptureButton::UpdateText() {
    if (capturing_) {
        setText("Press key...");
        return;
    }

    setText(QString::fromStdString(DisplayVirtualKeyName(virtual_key_)));
}

}  // namespace arrower
