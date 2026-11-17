// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
import QtQuick 1.0

Item {
    id: sw
    width: 92
    height: 44

    property bool checked: false
    signal toggled(bool value)

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        smooth: true
        color: sw.checked ? "#82D5C8" : "#333E3B"
        Behavior on color {
            ColorAnimation { duration: 140 }
        }
    }
    Rectangle {
        y: 4
        width: 36
        height: 36
        radius: 18
        smooth: true
        color: sw.checked ? "#181A1E" : "#C7CDD8"
        x: sw.checked ? sw.width - width - 4 : 4
        Behavior on x {
            NumberAnimation { duration: 140 }
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            sw.checked = !sw.checked;
            sw.toggled(sw.checked);
        }
    }
}
