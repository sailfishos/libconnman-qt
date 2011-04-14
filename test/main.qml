import Qt 4.7
import MeeGo.Connman 0.1

Item {
	//anchors.fill: parent
	width: 800
	height: 480

	NetworkListModel {
		id: networkListModel
	}

	Column {

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
