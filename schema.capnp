@0x87c48c9030332b1d;

interface Service {

	struct Info {
		uptime @0 :UInt64;
	}

	getInfo @0 () -> (info :Info);

	interface Channel {

		struct ChannelInfo {
			frequencyOffset @0 :Float32;
			timingOffset @1 :Float32;
			powerLevel @2 :Float32;
			isMuted @3 :Bool;
		}

		getInfo @0 () -> (info :ChannelInfo);
		mute @1 (mute :Bool) -> ();
	}

	listChannels @1 () -> (list :List(Channel));

	kill @2 () -> ();
}
