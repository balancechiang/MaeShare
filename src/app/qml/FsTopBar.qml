// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Header row for secondary pages: back chip + title + optional trailing chip.
import QtQuick 1.0

Item {
    id: bar
    property string title: ""
    property string actionText: ""
    property string backText: "Back"
    signal back()
    signal action()

    width: parent ? parent.width : 716
    height: 60

    FsIconChip {
        x: 8
        anchors.verticalCenter: parent.verticalCenter
        icon: "icons/arrow-back-accent.svg"
        fallbackText: bar.backText
        onClicked: bar.back()
    }
    Text {
        x: 104
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width - (bar.actionText.length > 0 ? 248 : 128)
        text: bar.title
        color: "#FFFFFF"
        font.pixelSize: 24
        font.bold: true
        elide: Text.ElideRight
    }
    FsChip {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        visible: bar.actionText.length > 0
        text: bar.actionText
        onClicked: bar.action()
    }
}
