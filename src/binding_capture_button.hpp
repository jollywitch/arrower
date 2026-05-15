#pragma once

#include <QPushButton>

namespace arrower {

class BindingCaptureButton : public QPushButton {
    Q_OBJECT

public:
    explicit BindingCaptureButton(QWidget* parent = nullptr);

    void SetVirtualKey(int virtual_key);
    int VirtualKey() const;

signals:
    void VirtualKeyChanged(int virtual_key);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void StartCapture();
    void StopCapture();
    void UpdateText();

    int virtual_key_ = 0;
    bool capturing_ = false;
};

}  // namespace arrower
