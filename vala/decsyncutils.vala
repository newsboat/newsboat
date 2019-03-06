public class Extra {

	public void* api;

	public Extra(void* api)
	{
		this.api = api;
	}
}

public delegate void SubscribeFunc(void* api, string feedUrl, bool subscribed);

class SubscriptionsListener : OnSubfileEntryUpdateListener<Extra> {

	private SubscribeFunc m_subscribe;

	public SubscriptionsListener(owned SubscribeFunc subscribe)
	{
		this.m_subscribe = (owned)subscribe;
	}

	public override Gee.List<string> subfile()
	{
		return toList({"feeds", "subscriptions"});
	}

	public override void onSubfileEntryUpdate(Decsync.Entry entry, Extra extra)
	{
		var feedUrl = entry.key.get_string();
		if (feedUrl == null) {
			Log.w("Invalid info key " + Json.to_string(entry.key, false));
			return;
		}
		var subscribed = entry.value.get_boolean();
		m_subscribe(extra.api, feedUrl, subscribed);
	}
}

public Decsync<Extra> getDecsync(string decsyncDir,
    owned SubscribeFunc subscribe)
{
	var decsyncSubdir = getDecsyncSubdir(decsyncDir, "rss");
	var ownAppId = getAppId("newsboat");
	var listeners = new Gee.ArrayList<OnEntryUpdateListener>();
	listeners.add(new SubscriptionsListener((owned)subscribe));
	return new Decsync<Extra>(decsyncSubdir, ownAppId, listeners);
}

public void markArticleRead(Decsync<Extra> decsync, int year, int month, int day, string guid, bool read)
{
	string[] path = {"article", year.to_string(), month.to_string(), day.to_string()};
	var key = stringToNode(guid);
	var value = boolToNode(read);
	decsync.setEntry(path, key, value);
}

public void executeAllNewEntries(Decsync<Extra> decsync, void* api)
{
	var extra = new Extra(api);
	decsync.executeAllNewEntries(extra);
}

public void executeStoredSubscriptions(Decsync<Extra> decsync, void* api)
{
	string[] path = {"feeds", "subscriptions"};
	var extra = new Extra(api);
	decsync.executeStoredEntries(path, extra);
}
