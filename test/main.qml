import Qt 4.7
import MeeGo.Connman 0.1

Item {
	//anchors.fill: parent
	width: 800
	height: 480

	NetworkListModel {
		id: networkListModel
	}
    TimezoneModel {
        id: timezoneModel
    }

	Column {

        Text { text: "Timezone: " + timezoneModel.timezone }
        Row {
            TextInput {
                id: timezoneEntry
                width: 200
                height: 40
                text: timezoneModel.timezone
            }
            Rectangle {
                id: setButton
                width: 40
                height: 40
                color: "grey"
                Text { anchors.centerIn: parent; text: "Set" }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        timezoneModel.timezone = timezoneEntry.text
                    }
                }
            }
        }
        Rectangle {
            id: updatesButton
            property string mode: timezoneModel.timezoneUpdates
            width: 180
            height: 50
            color: mode == "auto" ? "blue" : "red"
            Text {
                anchors.centerIn: parent
                text: "TimezoneUpdates: " + updatesButton.mode
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (updatesButton.mode == "auto")
                        timezoneModel.timezoneUpdates = "manual";
                    else
                        timezoneModel.timezoneUpdates = "auto";
                }
            }
        }
		Rectangle {
			id: button
			property bool airplane: networkListModel.offlineMode
			width: 100
			height: 50
			color: button.airplane ? "blue":"red"

			Text {
				anchors.centerIn: parent
				text: "offline: " + button.airplane
			}

			MouseArea {
				anchors.fill: parent
				onClicked: {
					networkListModel.offlineMode = !button.airplane
				}
			}
		}

		ListView {
			height: 600
			width: parent.width
			model: networkListModel
			delegate: Text {
				text: name + " " + type
			}
		}
	}
}
