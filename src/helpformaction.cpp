#include "helpformaction.h"

#include <cstring>

#include "config.h"
#include "fmtstrformatter.h"
#include "keymap.h"
#include "listformatter.h"
#include "strprintf.h"
#include "utils.h"
#include "view.h"

namespace newsboat {

HelpFormAction::HelpFormAction(View& vv,
	std::string formstr,
	ConfigContainer* cfg,
	const std::string& ctx)
	: FormAction(vv, formstr, cfg)
	, apply_search(false)
	, context(ctx)
	, textview("helptext", FormAction::f)
{
}

bool HelpFormAction::process_operation(Operation op,
	const std::vector<std::string>& /* args */,
	BindingType /*bindingType*/)
{
	bool quit = false;
	bool hardquit = false;
	switch (op) {
	case OP_QUIT:
		quit = true;
		break;
	case OP_HARDQUIT:
		hardquit = true;
		break;
	case OP_SEARCH: {
		std::vector<QnaPair> qna;
		qna.push_back(QnaPair(_("Search for: "), ""));
		this->start_qna(qna, QnaFinishAction::Search, &searchhistory);
	}
	break;
	case OP_CLEARFILTER:
		apply_search = false;
		do_redraw = true;
		break;
	default:
		return handle_textview_operations(textview, op);
	}
	if (hardquit) {
		while (v.formaction_stack_size() > 0) {
			v.pop_current_formaction();
		}
	} else if (quit) {
		v.pop_current_formaction();
	}
	return true;
}

void HelpFormAction::prepare()
{
	if (do_redraw) {
		recalculate_widget_dimensions();
		set_keymap_hints();

		const unsigned int width = textview.get_width();

		FmtStrFormatter fmt;
		fmt.register_fmt('N', PROGRAM_NAME);
		fmt.register_fmt('V', utils::program_version());
		set_title(fmt.do_format(cfg->get_configvalue("help-title-format"), width));

		ListFormatter listfmt;

		std::vector<std::string> colors = utils::tokenize(
				cfg->get_configvalue("search-highlight-colors"), " ");
		set_value("highlight", make_colorstring(colors));

		const auto should_bind_be_visible = [&](const HelpBindInfo& bind) {
			return !apply_search
				|| strcasestr(bind.key_sequence.c_str(), searchphrase.c_str()) != nullptr
				|| strcasestr(bind.op_name.value_or("").c_str(), searchphrase.c_str()) != nullptr
				|| strcasestr(bind.description.c_str(), searchphrase.c_str()) != nullptr;
		};

		const auto should_unused_be_visible = [&](const UnboundAction& unbound) {
			return !apply_search
				|| strcasestr(unbound.op_name.c_str(), searchphrase.c_str()) != nullptr
				|| strcasestr(unbound.description.c_str(), searchphrase.c_str()) != nullptr;
		};

		const auto should_macro_be_visible = [&](const HelpMacroInfo& macro) {
			return !apply_search
				|| strcasestr(macro.key_sequence.c_str(), searchphrase.c_str()) != nullptr
				|| strcasestr(macro.description.c_str(), searchphrase.c_str()) != nullptr;
		};

		const auto apply_highlights = [&](const std::string& line) {
			auto text = StflRichText::from_plaintext(line);
			if (apply_search && searchphrase.length() > 0) {
				text.highlight_searchphrase(searchphrase);
			}
			return text;
		};

		const auto help_info = v.get_keymap()->get_help_info(context);

		const auto& bindings = help_info.bindings;
		for (const auto& desc : bindings) {
			if (should_bind_be_visible(desc)) {
				const std::string line = strprintf::fmt("%-15s %-23s %s",
						desc.key_sequence,
						desc.op_name.value_or(""),
						desc.description);
				listfmt.add_line(apply_highlights(line));
			}
		}

		const auto& unused_actions = help_info.unused;
		if (!unused_actions.empty()) {
			listfmt.add_line(StflRichText::from_plaintext(""));
			listfmt.add_line(StflRichText::from_plaintext(_("Unbound functions:")));
			listfmt.add_line(StflRichText::from_plaintext(""));

			for (const auto& desc : unused_actions) {
				if (should_unused_be_visible(desc)) {
					const std::string line = strprintf::fmt("%-39s %s", desc.op_name, desc.description);
					listfmt.add_line(apply_highlights(line));
				}
			}
		}

		const auto& macros = help_info.macros;
		if (!macros.empty()) {
			listfmt.add_line(StflRichText::from_plaintext(""));
			listfmt.add_line(StflRichText::from_plaintext(_("Macros:")));
			listfmt.add_line(StflRichText::from_plaintext(""));

			for (const auto& macro : macros) {
				if (should_macro_be_visible(macro)) {
					const std::string line = strprintf::fmt("%s  %s", macro.key_sequence, macro.description);
					listfmt.add_line(apply_highlights(line));
				}
			}
		}

		textview.stfl_replace_lines(listfmt.get_lines_count(), listfmt.format_list());

		do_redraw = false;
	}
}

void HelpFormAction::init()
{
}

std::vector<KeyMapHintEntry> HelpFormAction::get_keymap_hint() const
{
	std::vector<KeyMapHintEntry> hints;
	hints.push_back({OP_QUIT, _("Quit")});
	hints.push_back({OP_SEARCH, _("Search")});
	if (apply_search) {
		hints.push_back({OP_CLEARFILTER, _("Clear")});
	}
	return hints;
}

void HelpFormAction::finished_qna(QnaFinishAction op)
{
	v.inside_qna(false);
	switch (op) {
	case QnaFinishAction::Search:
		searchphrase = qna_responses[0];
		apply_search = true;
		do_redraw = true;
		break;
	default:
		FormAction::finished_qna(op);
		break;
	}
}

std::string HelpFormAction::title()
{
	return _("Help");
}

std::string HelpFormAction::make_colorstring(
	const std::vector<std::string>& colors)
{
	std::string result;
	if (colors.size() > 0) {
		if (colors[0] != "default") {
			result.append("fg=");
			result.append(colors[0]);
		}
		if (colors.size() > 1) {
			if (colors[1] != "default") {
				if (result.length() > 0) {
					result.append(",");
				}
				result.append("bg=");
				result.append(colors[1]);
			}
		}
		for (unsigned int i = 2; i < colors.size(); i++) {
			if (result.length() > 0) {
				result.append(",");
			}
			result.append("attr=");
			result.append(colors[i]);
		}
	}
	return result;
}

} // namespace newsboat
