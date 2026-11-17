// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Settings line: label on the left, tappable value pill on the right.
import QtQuick 1.0

Item {
    id: row
    property string label: ""
    property string valueText: ""
    signal clicked()

    width: 372
    height: 68

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width - pill.width - 12
        text: row.label
        color: "#E2E4E9"
        font.pixelSize: 21
        elide: Text.ElideRight
    }

    Rectangle {
        id: pill
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Math.round(parent.width * 0.56)
        height: 50
        radius: 25
        smooth: true
        color: rowMouse.pressed ? "#3A5450" : "#333E3B"

        Text {
            anchors.centerIn: parent
            width: parent.width - 20
            text: row.valueText
            color: "#FFFFFF"
            font.pixelSize: 19
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
        }

        MouseArea {
            id: rowMouse
            anchors.fill: parent
            onClicked: row.clicked()
        }
    }
}
