// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Selection card on the send page: Material icon + label, side by side.
import QtQuick 1.0

Rectangle {
    id: card
    property string icon: ""
    property string title: ""
    property bool selected: false
    property bool enabledCard: true
    property int iconSize: 34
    signal clicked()

    width: 200
    height: 88
    radius: 16
    smooth: true
    color: cardMouse.pressed ? "#3A5450" : (selected ? "#2C4744" : "#2A3735")
    border.width: selected ? 2 : 0
    border.color: "#82D5C8"

    Behavior on color {
        ColorAnimation { duration: 100 }
    }

    Row {
        anchors.centerIn: parent
        spacing: 12

        Image {
            source: card.icon
            width: card.iconSize
            height: card.iconSize
            smooth: true
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: card.title
            color: "#FFFFFF"
            font.pixelSize: 21
            font.bold: true
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        enabled: card.enabledCard
        onClicked: card.clicked()
    }
}
