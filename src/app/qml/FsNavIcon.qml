// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
// Left navigation rail button: icon inside a rounded pill (no text glyphs).
import QtQuick 1.0

Rectangle {
    id: nav
    property string icon: ""
    property bool active: false
    signal clicked()

    width: 78
    height: 56
    radius: 28
    smooth: true
    color: nav.active ? "#2C4744" : (mouse.pressed ? "#2A303B" : "#00000000")

    Behavior on color {
        ColorAnimation { duration: 120 }
    }

    Image {
        anchors.centerIn: parent
        width: 44
        height: 44
        source: nav.icon
        smooth: true
        opacity: nav.active ? 1.0 : 0.70
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: nav.clicked()
    }
}
