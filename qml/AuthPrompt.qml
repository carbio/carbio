import QtQuick
import QtQuick.Controls
import "components/composites"

Rectangle {
    id: authPrompt
    anchors.fill: parent
    color: "#1E1E1E"
    layer.enabled: true
    layer.smooth: true
    layer.samples: 4

    // Signal emitted when hiding (for dashboard coordination)
    signal authPromptHiding()

    // Enum constants matching C++ AuthState
    readonly property int stateOFF: 0
    readonly property int stateSCANNING: 1
    readonly property int stateAUTHENTICATING: 2
    readonly property int stateALERT: 3
    readonly property int stateON: 4

    // Automotive color palette (ISO 2575 compliant)
    readonly property color colorNeutral: "#FFFFFF"    // White - normal status
    readonly property color colorActive: "#00A8FF"     // Blue - active/scanning
    readonly property color colorWarning: "#FFC107"    // Amber - warning
    readonly property color colorDanger: "#FF0000"     // Red - critical alert

    // Current authentication state
    property int authState: stateOFF
    property int failedAttempts: 0
    property int lockoutSeconds: 0
    property int maxLockoutSeconds: 20
    property string driverName: ""
    property bool isProcessing: false

    // Monitor state transitions to ensure clean resets
    onAuthStateChanged: {
        // Emit signal when transitioning to ON state (notify dashboard to prepare)
        if (authState === stateON) {
            authPromptHiding()
        }
    }

    // Visibility based on state
    visible: authState !== stateON
    opacity: authState !== stateON ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation {
            duration: 300
            easing.type: Easing.OutCubic
        }
    }

    // Transition state machine
    states: [
        State {
            name: "OFF"
            when: authState === stateOFF
            PropertyChanges { target: offStateItem; opacity: 1.0; scale: 1.0; visible: true; enabled: true }
            PropertyChanges { target: scanningStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: authenticatingStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: alertStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
        },
        State {
            name: "SCANNING"
            when: authState === stateSCANNING
            PropertyChanges { target: offStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: scanningStateItem; opacity: 1.0; scale: 1.0; visible: true; enabled: true }
            PropertyChanges { target: authenticatingStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: alertStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
        },
        State {
            name: "AUTHENTICATING"
            when: authState === stateAUTHENTICATING
            PropertyChanges { target: offStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: scanningStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: authenticatingStateItem; opacity: 1.0; scale: 1.0; visible: true; enabled: true }
            PropertyChanges { target: alertStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
        },
        State {
            name: "ALERT"
            when: authState === stateALERT
            PropertyChanges { target: offStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: scanningStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: authenticatingStateItem; opacity: 0.0; scale: 0.95; visible: false; enabled: false }
            PropertyChanges { target: alertStateItem; opacity: 1.0; scale: 1.0; visible: true; enabled: true }
        }
    ]

    // Instant transitions for real-time feedback
    transitions: [
        Transition {
            from: "*"
            to: "*"
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation {
                        properties: "opacity,scale"
                        duration: 300
                        easing.type: Easing.InOutCubic
                    }
                }
                PropertyAction { property: "visible" }
            }
        }
    ]

    // OFF State - Not used (system starts in SCANNING)
    Item {
        id: offStateItem
        anchors.fill: parent
        opacity: 0
        scale: 0.95
        visible: false
        enabled: false

        Label {
            anchors.centerIn: parent
            text: "System Off"
            font.pixelSize: 32
            font.family: "Inter"
            color: "#333333"
            opacity: 0.3
        }
    }

    // SCANNING State - Biometric ready indicator
    FingerprintScanner {
        id: scanningStateItem
        anchors.centerIn: parent
        opacity: 0
        scale: 0.95
        visible: false
        enabled: false
        failedAttempts: authPrompt.failedAttempts
        isScanning: opacity > 0.5 && enabled
    }

    AuthProgress {
        id: authenticatingStateItem
        anchors.centerIn: parent
        opacity: 0
        scale: 0.95
        visible: false
        enabled: false
        isProcessing: opacity > 0.5 && enabled
    }

    // ALERT State - Security lockout
    LockoutDisplay {
        id: alertStateItem
        anchors.centerIn: parent
        opacity: 0
        scale: 0.95
        visible: false
        enabled: false
        lockoutSeconds: authPrompt.lockoutSeconds
        maxLockoutSeconds: authPrompt.maxLockoutSeconds
        isLocked: opacity > 0.5 && enabled
    }

    // Success/Failure feedback overlay
    FeedbackDisplay {
        id: feedbackOverlay
        anchors.centerIn: parent
        z: 100
    }

    // Functions to trigger animations
    function showSuccess(name) {
        driverName = name
        feedbackOverlay.feedbackType = "success"
        feedbackOverlay.driverName = name
        feedbackOverlay.show()
    }

    function showFailure(attempts) {
        failedAttempts = attempts
        feedbackOverlay.feedbackType = "failure"
        feedbackOverlay.attempts = attempts
        feedbackOverlay.show()
    }

    // Lockout countdown is managed by C++ controller
    // No QML timer needed - controller handles onLockoutTick()
}
