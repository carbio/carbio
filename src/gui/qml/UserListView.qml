import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    signal deleteRequested(int userId, string userName)

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "Enrolled Users"
                font.pixelSize: 20
                font.family: "Inter"
                font.bold: Font.Bold
                color: "#FFFFFF"
            }

            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 30
                radius: 6
                color: controller.userManager.count > 0 ? "#1E3E1E" : "#3A2A2A"
                border.color: controller.userManager.count > 0 ? "#32D74B" : "#666666"
                border.width: 2

                Label {
                    anchors.centerIn: parent
                    text: controller.userManager.count
                    font.pixelSize: 16
                    font.family: "Courier New"
                    font.bold: Font.Bold
                    color: controller.userManager.count > 0 ? "#32D74B" : "#AAAAAA"
                }
            }

            Item { Layout.fillWidth: true }

            // Refresh button
            Rectangle {
                Layout.preferredWidth: 35
                Layout.preferredHeight: 35
                radius: 6
                color: refreshMouseArea.containsMouse ? "#3A4A4A" : "#2A3A3A"
                border.color: "#01E4E0"
                border.width: 1

                Behavior on color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: refreshMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: controller.userManager.refresh()
                }

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.strokeStyle = "#01E4E0"
                        ctx.lineWidth = 2
                        ctx.lineCap = "round"

                        var centerX = width / 2
                        var centerY = height / 2
                        var radius = 10

                        ctx.beginPath()
                        ctx.arc(centerX, centerY, radius, -Math.PI / 4, Math.PI * 1.5, false)
                        ctx.stroke()

                        ctx.beginPath()
                        ctx.moveTo(centerX + radius * 0.7, centerY - radius * 0.7)
                        ctx.lineTo(centerX + radius * 0.7 + 4, centerY - radius * 0.7 - 4)
                        ctx.stroke()

                        ctx.beginPath()
                        ctx.moveTo(centerX + radius * 0.7, centerY - radius * 0.7)
                        ctx.lineTo(centerX + radius * 0.7 + 4, centerY - radius * 0.7 + 4)
                        ctx.stroke()
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444444"
        }

        // Empty state
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1E1E1E"
            radius: 10
            border.color: "#444444"
            border.width: 1
            visible: controller.userManager.count === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 15

                Label {
                    text: "👤"
                    font.pixelSize: 64
                    opacity: 0.3
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "No users enrolled yet"
                    font.pixelSize: 18
                    font.family: "Inter"
                    font.bold: Font.DemiBold
                    color: "#AAAAAA"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Click 'Enroll Print' to add your first user"
                    font.pixelSize: 14
                    font.family: "Inter"
                    color: "#888888"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // User list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: controller.userManager.count > 0

            ListView {
                id: userListView
                model: controller.userManager
                spacing: 10

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 85
                    color: "#2A2A2A"
                    radius: 10
                    border.color: model.isAdmin ? "#FFD700" : "#444444"
                    border.width: model.isAdmin ? 2 : 1

                    Behavior on border.color { ColorAnimation { duration: 200 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 15

                        // Avatar circle with initials
                        Rectangle {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 60
                            radius: 30
                            color: model.roleColor

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    var name = model.name || "?"
                                    var parts = name.split(" ")
                                    if (parts.length >= 2) {
                                        return (parts[0][0] + parts[1][0]).toUpperCase()
                                    }
                                    return name.substring(0, 2).toUpperCase()
                                }
                                font.pixelSize: 20
                                font.bold: Font.Bold
                                color: "#1E1E1E"
                            }
                        }

                        // User info
                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            RowLayout {
                                spacing: 8

                                Label {
                                    text: model.name
                                    font.pixelSize: 16
                                    font.family: "Inter"
                                    font.bold: Font.DemiBold
                                    color: "#FFFFFF"
                                }

                                // Admin badge
                                Rectangle {
                                    visible: model.isAdmin
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 20
                                    radius: 4
                                    color: "#FFD700"

                                    Label {
                                        anchors.centerIn: parent
                                        text: "ADMIN"
                                        font.pixelSize: 9
                                        font.bold: Font.Bold
                                        font.family: "Inter"
                                        color: "#1E1E1E"
                                    }
                                }
                            }

                            Label {
                                text: model.roleName
                                font.pixelSize: 13
                                font.family: "Inter"
                                color: model.roleColor
                            }

                            RowLayout {
                                spacing: 5

                                Label {
                                    text: "ID: " + model.id
                                    font.pixelSize: 11
                                    font.family: "Courier New"
                                    color: "#888888"
                                }

                                Label {
                                    text: "•"
                                    font.pixelSize: 11
                                    color: "#666666"
                                }

                                Label {
                                    text: "Last: " + (model.lastAccess || "Never")
                                    font.pixelSize: 11
                                    font.family: "Inter"
                                    color: "#888888"
                                }
                            }
                        }

                        // Delete button
                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 8
                            color: deleteMouseArea.containsMouse ? "#4A2A2A" : "#3A2A2A"
                            border.color: deleteMouseArea.containsMouse ? "#FF6666" : "#666666"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            MouseArea {
                                id: deleteMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor

                                onClicked: {
                                    deleteRequested(model.id, model.name)
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                text: "🗑"
                                font.pixelSize: 20
                            }
                        }
                    }
                }
            }
        }
    }
}
