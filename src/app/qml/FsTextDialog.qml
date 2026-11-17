// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Text message dialog: selectable body + copy to clipboard (LocalSend text messages).
import QtQuick 1.0

Item {
    id: dlg
    anchors.fill: parent
    visible: false
    z: 100

    property string titleText: ""
    property string bodyText: ""
    property string acceptText: "OK"
    property string copyText: "Copy"

    signal closed()

    function open() { dlg.visible = true }
    function close() { dlg.visible = false }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.78

        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: 680
        height: 372
        radius: 16
        smooth: true
        color: "#22252A"
        border.color: "#3A5450"
        border.width: 1

        Column {
            x: 22
            y: 18
            width: parent.width - 44
            spacing: 12

            Text {
                width: parent.width
                text: dlg.titleText
                font.pixelSize: 26
                font.bold: true
                color: "#FFFFFF"
                elide: Text.ElideRight
            }

            Rectangle {
                width: parent.width
                height: 208
                radius: 12
                color: "#1B1E23"
                border.width: 1
                border.color: "#39414F"

                Flickable {
                    id: bodyFlick
                    anchors.fill: parent
                    anchors.margins: 12
                    contentWidth: width
                    contentHeight: bodyEdit.height
                    clip: true
                    flickableDirection: Flickable.VerticalFlick

                    // Qt 4.7 的 TextEdit 没有暴露内容高度，用它旁边一个同宽不可见的 Text 量高度
                    Text {
                        id: bodyMeasure
                        width: bodyFlick.width
                        visible: false
                        text: dlg.bodyText
                        font.pixelSize: 20
                        wrapMode: Text.WordWrap
                    }

                    // TextEdit（而不是 Text）：正文可以手指选中，再按下面的「复制」
                    TextEdit {
                        id: bodyEdit
                        width: bodyFlick.width
                        height: Math.max(bodyFlick.height, bodyMeasure.height + 10)
                        text: dlg.bodyText
                        readOnly: true
                        selectByMouse: true
                        color: "#E2E4E9"
                        selectionColor: "#46766F"
                        selectedTextColor: "#FFFFFF"
                        font.pixelSize: 20
                        wrapMode: TextEdit.Wrap
                    }
                }
            }

            Row {
                id: buttonRow
                width: parent.width
                spacing: 10

                FsChip {
                    width: 150
                    text: dlg.copyText
                    onClicked: {
                        var picked = bodyEdit.selectedText;
                        if (!picked || picked.length === 0) picked = dlg.bodyText;
                        transfer.copyText(picked);
                    }
                }

                FsButton {
                    width: buttonRow.width - 160
                    fontSize: 20
                    bold: true
                    primary: true
                    text: dlg.acceptText
                    onClicked: {
                        dlg.close();
                        dlg.closed();
                    }
                }
            }
        }
    }
}
