// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
import QtQuick 1.0

Rectangle {
    id: field
    property alias text: input.text
    property alias maximumLength: input.maximumLength
    property alias echoMode: input.echoMode
    property alias inputItem: input

    width: 300
    height: 56
    radius: 12
    smooth: true
    color: "#181A1E"
    border.color: input.activeFocus ? "#82D5C8" : "#3A5450"
    border.width: 1

    TextInput {
        id: input
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        height: 28
        font.pixelSize: 22
        color: "#FFFFFF"
        selectionColor: "#2F4E49"
        selectedTextColor: "#FFFFFF"
        clip: true
        selectByMouse: true
    }
}
