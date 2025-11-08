import QtQuick
import "../atoms"

Item {
    id: feedbackDisplay

    // Props
    property string feedbackType: "success"  // "success" or "failure"
    property int attempts: 0                 // Used for failure

    // Dimensions
    width: 300
    height: 300

    // Visibility
    visible: false
    opacity: 0

    Behavior on opacity { NumberAnimation { duration: feedbackType === "success" ? 300 : 200 } }

    // Signals
    signal animationComplete()

    // Functions
    function show() {
        visible = true
        opacity = 1
        if (feedbackType === "success") {
            successIcon.progress = 0
            hideTimer.interval = 2000
        } else {
            failureIcon.progress = 0
            hideTimer.interval = 1500
        }
        hideTimer.start()
    }

    function hide() {
        opacity = 0
        hideCompleteTimer.start()
    }

    Timer {
        id: hideTimer
        onTriggered: {
            opacity = 0
            hideCompleteTimer.start()
        }
    }

    Timer {
        id: hideCompleteTimer
        interval: 300
        onTriggered: {
            visible = false
            animationComplete()
        }
    }

    // Success animation
    Item {
        id: successAnimation
        anchors.centerIn: parent
        width: 300
        height: 300
        visible: feedbackType === "success"

        // Green success circle expanding
        Rectangle {
            id: successCircle
            anchors.centerIn: parent
            width: 200
            height: 200
            radius: 100
            color: "#32D74B"
            opacity: 0.2
            scale: 0

            NumberAnimation on scale {
                running: successAnimation.visible && feedbackDisplay.visible
                from: 0
                to: 1.5
                duration: 800
                easing.type: Easing.OutQuad
            }

            NumberAnimation on opacity {
                running: successAnimation.visible && feedbackDisplay.visible
                from: 0.4
                to: 0
                duration: 800
            }
        }

        // Checkmark
        CheckmarkCanvas {
            id: successIcon
            anchors.centerIn: parent
            strokeColor: "#32D74B"
            strokeWidth: 12

            NumberAnimation on progress {
                running: successAnimation.visible && feedbackDisplay.visible
                from: 0
                to: 1
                duration: 600
                easing.type: Easing.OutQuad
            }
        }

        // Outer ring completion
        Rectangle {
            anchors.centerIn: parent
            width: 200
            height: 200
            radius: 100
            color: "transparent"
            border.color: "#32D74B"
            border.width: 6
            scale: 0.8
            opacity: 0

            NumberAnimation on scale {
                running: successAnimation.visible && feedbackDisplay.visible
                from: 0.8
                to: 1.0
                duration: 400
                easing.type: Easing.OutBack
            }

            NumberAnimation on opacity {
                running: successAnimation.visible && feedbackDisplay.visible
                from: 0
                to: 1
                duration: 400
            }
        }
    }

    // Failure animation
    Item {
        id: failureAnimation
        anchors.centerIn: parent
        width: 300
        height: 300
        visible: feedbackType === "failure"

        // Red warning circle
        Rectangle {
            anchors.centerIn: parent
            width: 200
            height: 200
            radius: 100
            color: "#FF3333"
            opacity: 0.2
            scale: 0.8

            SequentialAnimation on scale {
                running: failureAnimation.visible && feedbackDisplay.visible
                NumberAnimation { from: 0.8; to: 1.1; duration: 200; easing.type: Easing.OutQuad }
                NumberAnimation { from: 1.1; to: 1.0; duration: 200; easing.type: Easing.InOutQuad }
            }

            SequentialAnimation on opacity {
                running: failureAnimation.visible && feedbackDisplay.visible
                NumberAnimation { from: 0.4; to: 0.2; duration: 200 }
                NumberAnimation { from: 0.2; to: 0.4; duration: 200 }
            }
        }

        // X mark
        XMarkCanvas {
            id: failureIcon
            anchors.centerIn: parent
            strokeColor: "#FF3333"
            strokeWidth: 14

            NumberAnimation on progress {
                running: failureAnimation.visible && feedbackDisplay.visible
                from: 0
                to: 1
                duration: 400
                easing.type: Easing.OutQuad
            }
        }

        // Outer warning ring
        Rectangle {
            anchors.centerIn: parent
            width: 200
            height: 200
            radius: 100
            color: "transparent"
            border.color: "#FF3333"
            border.width: 6
            opacity: 0

            SequentialAnimation on opacity {
                running: failureAnimation.visible && feedbackDisplay.visible
                NumberAnimation { from: 0; to: 1; duration: 300 }
                PauseAnimation { duration: 800 }
                NumberAnimation { from: 1; to: 0; duration: 300 }
            }
        }
    }
}
