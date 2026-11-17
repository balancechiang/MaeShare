// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Chih
import QtQuick 1.0

Item {
    id: cell
    width: 42
    height: 42

    property bool checked: false
    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: 9
        smooth: true
        color: cell.checked ? "#82D5C8" : "#252932"
        border.color: cell.checked ? "#82D5C8" : "#3A5450"
        border.width: 1
    }
    Rectangle {
        anchors.centerIn: parent
        width: 18
        height: 18
        radius: 5
        color: "#181A1E"
        visible: cell.checked
    }
    MouseArea {
        anchors.fill: parent
        onClicked: cell.clicked()
    }
}
