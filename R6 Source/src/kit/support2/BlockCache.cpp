
bool TCDApplication::QuitRequested(void)
{
	quitting = true;
	return true;
}

//--------------------------------------------------------------------

void TCDApplication::RefsReceived(BMessage *msg)
{
	int32		item = 0;
	entry_ref	ref;

	if (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		BEntry	entry(&ref);
		BFile	file;

		file.SetTo(&ref, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			BNodeInfo	node(&file);
			char		type[B_FILE_NAME_LENGTH];

			node.GetType(type);
			if (!strcmp(type, kCDDA_TYPE)) {
				attr_info	info;

				if (file.GetAttrInfo(kCDDA_ATTRIBUTE, &info) == B_NO_ERROR) {
					int32		index = 0;
					int32		track;
					BMenuItem	*item;
					fs_info		fs;

					file.ReadAttr(kCDDA_ATTRIBUTE, B_INT32_TYPE, 0, &track, info.size);
					fs_stat_dev(ref.device, &fs);
					while ((item = fDeviceMenu->ItemAt(index)) != NULL) {
						if (!strcmp(item->Label(), fs.device_name)) {
							item->SetMarked(true);
							FindPlayer(index, track);
							return;
						}
						index++;
					}

					if (file.GetAttrInfo(CD_KEY, &info) == B_NO_ERROR) {
						uint32	key;

						file.ReadAttr(CD_KEY, B_INT32_TYPE, 0, (int32 *)&key, info.size);
						FindDevice(key, track);
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------

void TCDApplication::FindDevice(uint32 key, int32 track)
{
	char			str[256];
	int32			fd;
	int32			index = 0;
	BEntry			entry;
	BFile			file;
	BMenuItem		*item;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	while ((item = fDeviceMenu->ItemAt(index)) != NULL) {
		if ((fd = open(item->Label(), O_RDWR)) >= 0) {
			scsi_toc	TOC;

			if (ioctl(fd, B_SCSI_GET_TOC, &TOC) == B_NO_ERROR) {
				char		byte;
				uint32		new_key;

				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 5];
				new_key = 0;
				new_key = (byte / 10) << 20;
				new_key |= (byte % 10) << 16;
				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 6];
				new_key |= (byte / 10) << 12;
				new_key |= (byte % 10) << 8;
				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 7];
				new_key |= (byte / 10) << 4;
				new_key |= byte % 10;
				if (key == new_key) {
					FindPlayer(index, track);
					close(fd);
					return;
				}
			}
			close(fd);
		}
		index++;
	}

	sprintf(str, "%s=%d", CD_KEY, key);
	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(str);
	query.Fetch();

	if (query.GetNextEntry(&entry) == B_NO_ERROR) {
		char	*error;
		char	*list;
		int32	len = 0;