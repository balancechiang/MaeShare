// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
import QtQuick 1.0

Rectangle {
    id: field
    property alias text: input.text
    property alias maximumLength: input.maximumLength
    property alias echoMode: input.echoMode
    property alias inputItem: input
    // Maemo 5 上点一下输入框并不会自动给 TextInput 焦点（scene_focus_item 一直是 nil），
    // 这里显式抢焦点：页面切入时一次（autoFocus），点击时再一次。
    property bool autoFocus: false

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

    // 透明覆盖层：整块区域点击都把焦点交给输入框（放在 TextInput 之后才在上层）。
    MouseArea {
        anchors.fill: parent
        onClicked: input.forceActiveFocus()
    }

    Component.onCompleted: {
        if (autoFocus) input.forceActiveFocus();
    }
}
