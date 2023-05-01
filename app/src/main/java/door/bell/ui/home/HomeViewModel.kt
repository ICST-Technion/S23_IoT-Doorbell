package door.bell.ui.home

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel

class HomeViewModel(private val userName: String) : ViewModel() {

    private val _text = MutableLiveData<String>().apply {
        value = "Welcome back $userName!"
    }
    val text: LiveData<String> = _text
}
