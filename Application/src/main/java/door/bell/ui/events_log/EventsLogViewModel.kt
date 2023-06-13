package door.bell.ui.events_log

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class EventsLogViewModel : ViewModel() {

    private val _text = MutableLiveData<String>().apply {
        value = "This is events_log Fragment"
    }
    val text: LiveData<String> = _text
}